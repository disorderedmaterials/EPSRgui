#include <QtGui>
#include <QPushButton>
#include <QDebug>
#include <QProcess>
#include <QString>
#include <QTableWidget>

#include "epsrproject.h"
#include "molfiles.h"
#include "moloptionsdialog.h"
#include "makeatomdialog.h"
#include "makelatticedialog.h"

void MainWindow::on_createMolFileButton_clicked(bool checked)
{
   if (!molOptionsDialog)
    {
        molOptionsDialog = new MolOptionsDialog(this);
    }

    molOptionsDialog->setModal(true);
    molOptionsDialog->show();
    molOptionsDialog->raise();
    molOptionsDialog->activateWindow();

    molDialog = molOptionsDialog->exec();

    if (molDialog == MolOptionsDialog::Accepted)
    {
        molCharge = molOptionsDialog->getMolCharge();
        mopacOption = molOptionsDialog->getMopacOptions();
        ui.messagesLineEdit->setText("Running Jmol");

        QString program = epsrBinDir_+"Jmol.jar";

        // set jmol save directory to EPSR directory
        QDir::setCurrent(workingDir_);
        QProcess processJmol;
        processJmol.startDetached("java", QStringList() << "-jar" << program);

        //add path to QFileSystemWatcher
        jmolFile_.addPath(workingDir_);
        QDir dir;
        dir.setPath(workingDir_);
        QStringList jmolFilter;
        jmolFilter << "*.jmol";
        QStringList jmolFiles = dir.entryList(jmolFilter, QDir::Files);
        jmolFile_.addPaths(jmolFiles);
    }
}

void MainWindow::makeMolFile()
{
    //find most recently modified file and check if it is a .jmol file
    QDir dir;
    dir.setPath(workingDir_);
    dir.setSorting(QDir::Time);
    QStringList jmolFilter;
    jmolFilter << "*.jmol";
    QStringList jmolFiles = dir.entryList(jmolFilter, QDir::Files);

    if (jmolFiles.count() == 0)
    {
        return;
    }

    //check the file was modified in the last 2 seconds
    QFileInfo fi(jmolFiles.at(0));
    if (fi.lastModified().msecsTo(QDateTime::currentDateTime()) > 2000 )
    {
        return;
    }

    //stop QFileSystemWatcher so don't get double ups message
    jmolFile_.removePath(workingDir_);
    foreach (QString jmolFile, jmolFiles)
    {
        jmolFile_.removePath(jmolFile);
    }

    QString jmolFileName = jmolFiles.at(0);

    //check if a .mol file of the same name is already listed as a component
    QString jmolBaseFileName=jmolFileName.split(".",QString::SkipEmptyParts).at(0);
    for (int i = 0; i < ui.molFileList->count(); i++)
    {
        if (ui.molFileList->item(i)->text() == jmolBaseFileName+".mol")
        {
            QMessageBox msgBox;
            msgBox.setText("This .mol file is already listed as a component.");
            msgBox.exec();
            return;
        }
    }

    ui.messagesLineEdit->setText("Making .mol and .ato files");  

    QString mopacOptionStr = QString::number(mopacOption);
    QString molChargeStr = QString::number(molCharge);

    // Run MOPAC if mopacOption is selected
    if (mopacOption != 0)
    {
#ifdef _WIN32
        QFile jmolBatFile(workingDir_+"mopac"+jmolBaseFileName+".bat");
#else
        QFile jmolBatFile(workingDir_+"mopac"+jmolBaseFileName+".sh");
#endif
        if(!jmolBatFile.open(QFile::WriteOnly | QFile::Text))
        {
            QMessageBox msgBox;
            msgBox.setText("Could not make MOPAC setup file.");
            msgBox.exec();
            return;
        }

        QTextStream stream(&jmolBatFile);
#ifdef _WIN32
        stream << "set EPSRbin=" << epsrBinDir_ << "\n"
               << "set EPSRrun=" << workingDir_ << "\n"
               << "CD " << workingDir_ << "\n"
               << "%EPSRbin%readmole.exe " << workingDir_ << " readmole .jmol " << jmolBaseFileName << "\n";
#else
        stream << "export EPSRbin=" << epsrBinDir_ << "\n"
               << "export EPSRrun=" << workingDir_ << "\n"
               << "cd " << workingDir_ << "\n"
               << "\"$EPSRbin\"'readmole' " << workingDir_ << " readmole .jmol " << jmolBaseFileName << "\n";
#endif
        jmolBatFile.close();

#ifdef _WIN32
        processEPSR_.start("mopac"+jmolBaseFileName+".bat");
#else
        processEPSR_.start("sh mopac"+jmolBaseFileName+".sh");
#endif

        if (!processEPSR_.waitForStarted()) return;

        processEPSR_.write("3\n");          // select bonding patterns and changelabel section

        processEPSR_.write("y\n");          //yes to running mopac

        processEPSR_.write(qPrintable(mopacOptionStr+"\n"));    //mopac option number

        processEPSR_.write(qPrintable(molChargeStr+"\n"));      //molecular charge

        if (!processEPSR_.waitForFinished()) return;

        messageText_ += "\nMOPAC finished - check "+jmolBaseFileName+".out file for results of calculation.\n";
        messagesDialog.refreshMessages();
    }
    else
    {
#ifdef _WIN32
        processEPSR_.start(epsrBinDir_+"readmole.exe", QStringList() << workingDir_ << "jmolfile "+jmolFileName);
#else
        processEPSR_.start(epsrBinDir_+"readmole", QStringList() << workingDir_ << "jmolfile "+jmolFileName);
#endif
        if (!processEPSR_.waitForStarted()) return;

        processEPSR_.write("3\n");          // select bonding patterns and changelabel section

        processEPSR_.write("n\n");    //don't run mopac here as calls to "%EPSRbin%" in path which obviously isn't defined!

        if (!processEPSR_.waitForFinished()) return;
    }

    // if a new .mol file is created, load this into ui.molFileList:
    QDir dir2;
    dir2.setSorting(QDir::Time);
    QStringList molFilter;
    molFilter << "*.mol";
    QStringList molFiles = dir.entryList(molFilter, QDir::Files);
    if (molFiles.isEmpty())
    {
        ui.messagesLineEdit->setText("No .mol files found");
        return;
    }
    QString molFileName = molFiles.at(0);
    QFileInfo molFileInfo(molFileName);
    QDateTime molModTime;
    molModTime = molFileInfo.lastModified();
    QDateTime dateTimeNow2 = QDateTime::currentDateTime();
    if (molModTime < dateTimeNow2.addSecs(-30))
    {
        ui.messagesLineEdit->setText("No new .mol files found");
        return;
    }
    molFileName_= molFileName;
    QString cropped_fileName = molFileName_.split(".",QString::SkipEmptyParts).at(0);
    QString atoFileName = cropped_fileName+".ato";

    //update mol file table
    ui.molFileList->addItem(molFileName_);
    nMolFiles = ui.molFileList->count();

    //update ato file table in box ato tab
    ui.atoFileTable->setRowCount(nMolFiles);
    if (nMolFiles > 0)
    {
        QTableWidgetItem *item = new QTableWidgetItem(atoFileName);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        ui.atoFileTable->setItem(nMolFiles-1,0, item);
        ui.atoFileTable->setItem(nMolFiles-1,2, new QTableWidgetItem("0"));
    }

    //save .pro file
    save();

    //set current mol file (reads mol file and updates bond distance/angles/etc tables
    ui.molFileList->setCurrentRow(nMolFiles-1);

    ui.molFileTabWidget->setEnabled(true);
    ui.viewMolFileButton->setEnabled(true);
    ui.removeMolFileButton->setEnabled(true);
    ui.updateMolFileButton->setEnabled(true);
    ui.mixatoButton->setEnabled(true);
    ui.addatoButton->setEnabled(true);
    ui.loadBoxButton->setEnabled(true);

    //restart QFileSystemWatcher
    jmolFile_.addPath(workingDir_);
    jmolFile_.addPaths(jmolFiles);

    ui.messagesLineEdit->setText("Finished making .mol and .ato files");
}

void MainWindow::on_molFileLoadButton_clicked(bool checked)
{
    QDir::setCurrent(workingDir_);

    QString molFile = QFileDialog::getOpenFileName(this, "Choose EPSR .mol file", workingDir_, tr(".mol files (*.mol)"));
    if (!molFile.isEmpty())
    {
        QString molFilePath = QFileInfo(molFile).path()+"/";
        molFilePath = QDir::toNativeSeparators(molFilePath);
        QFileInfo fi(molFile);
        QString molFileName = fi.fileName();

        //check if .mol file is already listed as a component
        for (int i = 0; i < ui.molFileList->count(); i++)
        {
            if (ui.molFileList->item(i)->text() == molFileName)
            {
                QMessageBox msgBox;
                msgBox.setText("This .mol file is already listed as a component.");
                msgBox.exec();
                return;
            }
        }

        //check if .mol file is an EPSR .mol file (and not an MDL .mol file)
        QFile file(molFile);
        if(!file.open(QFile::ReadOnly | QFile::Text))
        {
            QMessageBox msgBox;
            msgBox.setText("Could not open .mol file");
            msgBox.exec();
            return;
        }
        QTextStream stream(&file);
        QString line;
        QStringList dataLine;
        dataLine.clear();

        int molFormat = 0;
        do
        {
            line = stream.readLine();
            dataLine = line.split(" ", QString::SkipEmptyParts);
            if (dataLine.count() != 0)
            {
                if (dataLine.at(0).contains("potential"))
                {
                    molFormat = 1;
                }
            }
        } while (!line.isNull());
        file.close();

        if (molFormat == 0)
        {
            QMessageBox msgBox;
            msgBox.setText("This .mol file is not in the EPSR format.");
            msgBox.exec();
            return;
        }

        //if necessary, copy to workingDir_ (question if a file of the same name is already present there)
        if (molFilePath != workingDir_)
        {
            if (QFile::exists(workingDir_+molFileName) == true)
            {
                QMessageBox::StandardButton msgBox;
                msgBox  = QMessageBox::question(this, "Warning", "This will overwrite the .mol file already present in the project folder with the same name.\nProceed?", QMessageBox::Ok|QMessageBox::Cancel);
                if (msgBox == QMessageBox::Cancel)
                {
                    return;
                }
                else
                {
                    QFile::copy(molFile, workingDir_+molFileName);
                }
            }
            else
            {
                QFile::copy(molFile, workingDir_+molFileName);
            }
        }

        molFileName_= molFileName;
        QString cropped_fileName = molFileName_.split(".",QString::SkipEmptyParts).at(0);
        QString atoFileName = cropped_fileName+".ato";

        //check if equivalent .ato file also exists and, if not, run makemole to generate it
        if (QFile::exists(atoFileName) == 0)
        {
            QString projDir = workingDir_;
            projDir = QDir::toNativeSeparators(projDir);
#ifdef _WIN32
            processEPSR_.start(epsrBinDir_+"makemole.exe", QStringList() << projDir << "makemole" << cropped_fileName);
#else
            processEPSR_.start(epsrBinDir_+"makemole", QStringList() << projDir << "makemole" << cropped_fileName);
#endif
            if (!processEPSR_.waitForStarted()) return;
            if (!processEPSR_.waitForFinished()) return;
        }

        //update mol file table
        ui.molFileList->addItem(molFileName);
        nMolFiles = ui.molFileList->count();

        //update ato file table in box ato tab
        ui.atoFileTable->setRowCount(nMolFiles);
        if (nMolFiles > 0)
        {
            QTableWidgetItem *item = new QTableWidgetItem(atoFileName);
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            ui.atoFileTable->setItem(nMolFiles-1,0, item);
            ui.atoFileTable->setItem(nMolFiles-1,2, new QTableWidgetItem("0"));
        }

        //save .pro file
        save();

        //set current mol file (reads mol file and updates bond distance/angles/etc tables
        ui.molFileList->setCurrentRow(nMolFiles-1);

        ui.molFileTabWidget->setEnabled(true);
        ui.viewMolFileButton->setEnabled(true);
        ui.removeMolFileButton->setEnabled(true);
        ui.updateMolFileButton->setEnabled(true);
        ui.mixatoButton->setEnabled(true);
        ui.addatoButton->setEnabled(true);
        ui.loadBoxButton->setEnabled(true);
    }
}

void MainWindow::on_createAtomButton_clicked(bool checked)
{
    if (!makeAtomDialog)
    {
        makeAtomDialog = new MakeAtomDialog(this);
    }

    makeAtomDialog->setModal(true);
    makeAtomDialog->show();
    makeAtomDialog->raise();
    makeAtomDialog->activateWindow();

    atomDialog = makeAtomDialog->exec();

    if (atomDialog == MakeAtomDialog::Accepted)
    {
        QString atomName = makeAtomDialog->getName();
        QString atomEpsilon = makeAtomDialog->getEpsilon();
        QString atomSigma = makeAtomDialog->getSigma();
        QString atomMass = makeAtomDialog->getMass();
        QString atomCharge = makeAtomDialog->getCharge();
        QString atomSymbol = makeAtomDialog->getSymbol();

        QString projDir = workingDir_;
        projDir = QDir::toNativeSeparators(projDir);
#ifdef _WIN32
        processEPSR_.start(epsrBinDir_+"makeato.exe", QStringList() << projDir << "makeato" );
#else
        processEPSR_.start(epsrBinDir_+"makeato", QStringList() << projDir << "makeato" );
#endif
        if (!processEPSR_.waitForStarted()) return;

        processEPSR_.write(qPrintable(atomName+"\n"));

        processEPSR_.write(qPrintable(atomEpsilon+"\n"));

        processEPSR_.write(qPrintable(atomSigma+"\n"));

        processEPSR_.write(qPrintable(atomMass+"\n"));

        processEPSR_.write(qPrintable(atomCharge+"\n"));

        processEPSR_.write(qPrintable(atomSymbol+"\n"));

        processEPSR_.write(qPrintable("300\n"));

        processEPSR_.write(qPrintable("0.1\n"));

        if (!processEPSR_.waitForFinished()) return;

        messageText_ += "\nmakeato finished\n";
        QString atoFileName = atomName+".ato";
        QString molFileName = atomName+".mol";
        molFileName_= workingDir_+molFileName;

        //make an equivalent .mol file
        QFile fileRead(workingDir_+atoFileName);
        if(!fileRead.open(QFile::ReadOnly | QFile::Text))
        {
            QMessageBox msgBox;
            msgBox.setText("Could not open atom .ato file");
            msgBox.exec();
            return;
        }

        QFile fileWrite(molFileName_);
        if(!fileWrite.open(QFile::WriteOnly | QFile::Text))
        {
            QMessageBox msgBox;
            msgBox.setText("Could not create .mol file");
            msgBox.exec();
            return;
        }

        QTextStream streamRead(&fileRead);
        QTextStream streamWrite(&fileWrite);
        QString lineRead;
        QStringList dataLineRead;
        dataLineRead.clear();

        lineRead = streamRead.readLine();
        lineRead = streamRead.readLine();
        lineRead = streamRead.readLine();
        lineRead = streamRead.readLine();
        lineRead = streamRead.readLine();

        streamWrite << "  .gmol           0\n"
                    << " atom            1  001   " << lineRead << "           0\n"
                    << "changelabel 001 " << atomName << "\n";

        lineRead = streamRead.readLine();
        lineRead = streamRead.readLine();
        lineRead = streamRead.readLine();
        lineRead = streamRead.readLine();
        dataLineRead=lineRead.split(" ", QString::SkipEmptyParts);
        if (dataLineRead.count() == 0) return;

        streamWrite << "potential " << atomName << " " << dataLineRead.at(0) << "  " << dataLineRead.at(1) << "  " << dataLineRead.at(2) << "  " << dataLineRead.at(3) << " " << atomSymbol << "\n"
                    << "temperature 0.300000E+03\n"
                    << "vibtemp  0.650000E+02\n"
                    << "angtemp  0.100000E+01\n"
                    << "dihtemp  0.100000E+02\n"
                    << "ecoredcore    1.00000    2.00000\n"
                    << "density  0.1\n";

        fileRead.close();
        fileWrite.close();

        //if not already listed, update mol file table
        for (int i = 0; i < ui.molFileList->count(); i++)
        {
            if (ui.molFileList->item(i)->text() == molFileName)
            {
                return;
            }
        }
        ui.molFileList->addItem(molFileName);
        nMolFiles = ui.molFileList->count();

        //update ato file table in box ato tab
        ui.atoFileTable->setRowCount(nMolFiles);
        if (nMolFiles > 0)
        {
            QTableWidgetItem *item = new QTableWidgetItem(atoFileName);
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            ui.atoFileTable->setItem(nMolFiles-1,0, item);
            ui.atoFileTable->setItem(nMolFiles-1,2, new QTableWidgetItem("0"));
        }

        //save .pro file
        save();

        //set current mol file (reads mol file and updates bond distance/angles/etc tables
        ui.molFileList->setCurrentRow(nMolFiles-1);

        ui.molFileTabWidget->setEnabled(true);
        ui.viewMolFileButton->setEnabled(true);
        ui.removeMolFileButton->setEnabled(true);
        ui.updateMolFileButton->setEnabled(true);
        ui.mixatoButton->setEnabled(true);
        ui.addatoButton->setEnabled(true);
        ui.loadBoxButton->setEnabled(true);

        messagesDialog.refreshMessages();
        ui.messagesLineEdit->setText("Finished making new atom");
    }
}

void MainWindow::on_createLatticeButton_clicked(bool checked)
{
    //make the .ato file for the lattice (the lattice can only be single atoms as 'molecules' not multiatom moleucles)
    makeLatticeDialog = new MakeLatticeDialog(this);

    makeLatticeDialog->setModal(true);
    makeLatticeDialog->show();
    makeLatticeDialog->raise();
    makeLatticeDialog->activateWindow();

    latticeDialog = makeLatticeDialog->exec();

    if (latticeDialog == MakeLatticeDialog::Accepted)
    {
        QString unitFileName = makeLatticeDialog->unitFileName(); //this has the .unit extension on it
        QString croppedUnitFileName = unitFileName.split(".", QString::SkipEmptyParts).at(0);
        QString atoFileName = croppedUnitFileName+".ato";

        QString aLatt = makeLatticeDialog->cellsAlongA();
        QString bLatt = makeLatticeDialog->cellsAlongB();
        QString cLatt = makeLatticeDialog->cellsAlongC();

        QString projDir = workingDir_;
        projDir = QDir::toNativeSeparators(projDir);
#ifdef _WIN32
        processEPSR_.start(epsrBinDir_+"makelattice.exe", QStringList() << projDir << "makelattice" );
#else
        processEPSR_.start(epsrBinDir_+"makelattice", QStringList() << projDir << "makelattice" );
#endif
        if (!processEPSR_.waitForStarted()) return;

        processEPSR_.write(qPrintable(unitFileName+"\n"));

        processEPSR_.write(qPrintable(aLatt+" "+bLatt+" "+cLatt+"\n"));

        processEPSR_.write(qPrintable(croppedUnitFileName+"\n"));

        if (!processEPSR_.waitForFinished()) return;

        messageText_ += "\nmakelattice finished\n";

        // if use lattice as box
        if (makeLatticeDialog->useAsBox() == 1)
        {
            //make the .mol files for each atom in the lattice
            QStringList atomTypes = makeLatticeDialog->atomTypes(); //get list of atom labels from paramTable

            //read in .unit file to get epsilon, sigma, amu, charge, atom symbol for each atom type and write out to each .mol file
            QFile file(workingDir_+unitFileName);
            if(!file.open(QFile::ReadOnly | QFile::Text))
            {
                QMessageBox msgBox;
                msgBox.setText("Could not open .unit file");
                msgBox.exec();
                return;
            }

            QTextStream stream(&file);
            QString line;
            QStringList dataLine;
            dataLine.clear();

            line = stream.readLine();
            dataLine = line.split(" ", QString::SkipEmptyParts);
            if (dataLine.count() == 0)
            {
                QMessageBox msgBox;
                msgBox.setText("Could not read .unit file");
                msgBox.exec();
                return;
            }
            line = stream.readLine();
            line = stream.readLine();
            line = stream.readLine();
            int numberAtoms = line.toInt();
            for (int i = 0; i < numberAtoms; i++)
            {
                line = stream.readLine();
            }

            QStringList atomCharges;
            atomCharges.clear();

            //clear molFileList as it won't already be in the box
            ui.molFileList->clear();

            for (int i = 0; i < atomTypes.count(); i++)
            {
                QString molFileName = atomTypes.at(i);
                QFile fileWrite(workingDir_+molFileName+".mol");
                if(!fileWrite.open(QFile::WriteOnly | QFile::Text))
                {
                    QMessageBox msgBox;
                    msgBox.setText("Could not create .mol file");
                    msgBox.exec();
                    return;
                }

                line = stream.readLine();
                dataLine = line.split(" ", QString::SkipEmptyParts);
                QString symbol = dataLine.at(1);
                line = stream.readLine();
                dataLine = line.split(" ", QString::SkipEmptyParts);
                QString epsilon = dataLine.at(0);
                QString sigma = dataLine.at(1);
                QString mass = dataLine.at(2);
                QString charge = dataLine.at(3);
                atomCharges.append(dataLine.at(3));

                QTextStream streamWrite(&fileWrite);
                streamWrite << "  .gmol           0\n"
                            << " atom            1  001     0.00000E+00  0.00000E+00  0.00000E+00           0\n"
                            << "changelabel 001 " << atomTypes.at(i) << "\n"
                            << "potential " << atomTypes.at(i) << " " << epsilon << " " << sigma << " " << mass << " " << charge << " " << symbol << "\n"
                            << "temperature 300\n"
                            << "vibtemp  0.650000E+02\n"
                            << "angtemp  0.100000E+01\n"
                            << "dihtemp  0.100000E+02\n"
                            << "ecoredcore    0.00000    0.00000\n"
                            << "density  0.1\n";
                fileWrite.close();          

                QString projDir = workingDir_;
                projDir = QDir::toNativeSeparators(projDir);
#ifdef _WIN32
                processEPSR_.start(epsrBinDir_+"makemole.exe", QStringList() << projDir << "makemole" << molFileName);
#else
                processEPSR_.start(epsrBinDir_+"makemole", QStringList() << projDir << "makemole" << molFileName);
#endif
                if (!processEPSR_.waitForStarted()) return;
                if (!processEPSR_.waitForFinished()) return;

                ui.molFileList->addItem(molFileName+".mol");
            }
            file.close();

            //update lattice.ato with correct .mol files at bottom
            QFile fileato(workingDir_+atoFileName);
            if(!fileato.open(QFile::ReadWrite | QFile::Text))
            {
                QMessageBox msgBox;
                msgBox.setText("Could not open lattice .ato file");
                msgBox.exec();
                return;
            }

            QTextStream streamato(&fileato);
            QString lineato;
            QStringList dataLineato;
            dataLineato.clear();
            QString originalato;
            int ctr_moltype = 0;

            while (!streamato.atEnd())
            {
                lineato = streamato.readLine();
                originalato.append(lineato+"\n");
                if (lineato.isEmpty()) break;
                if (lineato.contains("moltype"))
                {
                    originalato.remove(lineato+"\n");
                    dataLineato = lineato.split(" ", QString::SkipEmptyParts);
                    originalato.append("   "+dataLineato.at(0)+"  "+atomTypes.at(ctr_moltype)+"  "+dataLineato.at(2)+"  "+dataLineato.at(3)+"  "+dataLineato.at(4)+"  "+dataLineato.at(5)+"\n");
                    ctr_moltype++;
                }
            }
            fileato.resize(0);
            streamato << originalato;
            fileato.close();

            //change filename to be appropriate and update atoFileName_
            QFile::rename(workingDir_+atoFileName, workingDir_+projectName_+"box.ato");
            QFile::remove(workingDir_+atoFileName);
            atoFileName_ = projectName_+"box.ato";

            ui.boxAtoLabel->setText(atoFileName_);
            readAtoFileBoxDetails();
            checkBoxCharge();

            //update atoFileTable to contain atom .ato files
            ui.atoFileTable->setRowCount(atomTypes.count());
            for (int i = 0; i < atomTypes.count(); i++)
            {
                QTableWidgetItem *item = new QTableWidgetItem(atomTypes.at(i)+".ato");
                item->setFlags(item->flags() & ~Qt::ItemIsEditable);
                ui.atoFileTable->setItem(i,0, item);
                ui.atoFileTable->setItem(i,1, new QTableWidgetItem(atomCharges.at(i)));
                ui.atoFileTable->setItem(i,2, new QTableWidgetItem(QString::number(numberAtomTypes.at(i))));
            }

            ui.deleteBoxAtoFileAct->setDisabled(false);
            ui.updateAtoFileButton->setDisabled(false);
            ui.removeComponentButton->setDisabled(false);
            ui.atoEPSRButton->setDisabled(false);
            ui.reloadBoxButton->setDisabled(false);
            ui.boxCompositionButton->setDisabled(false);
            ui.randomiseButton->setDisabled(false);
            ui.fmoleButton->setDisabled(false);
            ui.dataFileBrowseButton->setDisabled(false);
        }

        // otherwise use lattice as a component **NB** This is the only workflow that doesn't use a .mol file.
        else
        {
            //check .ato file doesn't already exist and if not, add .ato to molFileList
            if (ui.molFileList->count() > 0)
            {
                for (int i = 0; i < ui.molFileList->count(); i++)
                {
                    if (ui.atoFileTable->item(i,0)->text() == atoFileName)
                    {
                        QMessageBox msgBox;
                        msgBox.setText("A component with this name is already listed.\n This process will be aborted. Remake the lattice with a different name.");
                        msgBox.exec();
                        return;
                    }
                }
            }
            ui.molFileList->addItem(atoFileName);
            nMolFiles = ui.molFileList->count();

            //add .ato to atoFileTable
            ui.atoFileTable->setRowCount(nMolFiles);
            if (nMolFiles > 0)
            {
                QTableWidgetItem *item = new QTableWidgetItem(atoFileName);
                item->setFlags(item->flags() & ~Qt::ItemIsEditable);
                ui.atoFileTable->setItem(nMolFiles-1,0, item);

//                //calculate charge for ato file
//                QString lattCharge = makeLatticeDialog->charge();    //THIS ISNT WORKING **********************************************************************************
//                QTableWidgetItem *item1 = new QTableWidgetItem(lattCharge);
                QTableWidgetItem *item1 = new QTableWidgetItem("0");
                item1->setFlags(item1->flags() & ~Qt::ItemIsEditable);
                ui.atoFileTable->setItem(nMolFiles-1,1, item1);

                //put 1 in 'number in box' column and disable
                QTableWidgetItem *item2 = new QTableWidgetItem("1");
                item2->setFlags(item2->flags() & ~Qt::ItemIsEditable);
                ui.atoFileTable->setItem(nMolFiles-1,2, item2);
                //disable mixato, only allow addato
                ui.mixatoButton->setDisabled(true);
                ui.loadBoxButton->setDisabled(true);
            }
        }

        //save .pro file
        save();

        //set mol file table to select last item and update mol tables(runs via 'selectionChanged' signal)
        nMolFiles = ui.molFileList->count();
        ui.molFileList->setCurrentRow(nMolFiles-1);

        ui.molFileTabWidget->setEnabled(true);
        ui.viewMolFileButton->setEnabled(true);
        ui.removeMolFileButton->setEnabled(true);
        ui.updateMolFileButton->setEnabled(true);
        ui.mixatoButton->setEnabled(true);
        ui.addatoButton->setEnabled(true);

        messagesDialog.refreshMessages();
        ui.messagesLineEdit->setText("Finished making new lattice");
    }
}

void MainWindow::on_makeMolExtButton_clicked(bool checked)
{
    if (visualiserExe_.isEmpty())
    {
        QMessageBox msgBox;
        msgBox.setText("Please set an EPSR (.ato and .mol) compatible visualiser in Settings.");
        msgBox.exec();
        return;
    }

    QDir::setCurrent(workingDir_);

    QProcess processAten;
    processAten.setProcessChannelMode(QProcess::ForwardedChannels);
    processAten.startDetached(visualiserExe_, QStringList() << atoFileName_);
    ui.messagesLineEdit->setText("Alternative visualiser started");
}

void MainWindow::setSelectedMolFile()
{
    int row = ui.molFileList->currentRow();
    QString molFileName = ui.molFileList->item(row)->text();
    molFileName_ = molFileName;

    QString extension = molFileName_.split(".",QString::SkipEmptyParts).at(1);
    if (extension == "ato")
    {
        readAtoFile();
    }
    else
    {
        readMolFile();
    }
}

void MainWindow::on_viewMolFileButton_clicked(bool checked)
{    
    if (!visualiserExe_.isEmpty())
    {
        QProcess processAten;
        QString cropped_fileName = molFileName_.split(".",QString::SkipEmptyParts).at(0);
        QString atoFileName = cropped_fileName+".ato";
        processAten.setProcessChannelMode(QProcess::ForwardedChannels);
        processAten.startDetached(visualiserExe_, QStringList() << atoFileName);
        ui.messagesLineEdit->setText(".mol file opened");
    }
    else
    {
        QString atoBaseFileName = molFileName_.split(".",QString::SkipEmptyParts).at(0);

#ifdef _WIN32
        QFile jmolFile(workingDir_+"plot"+atoBaseFileName+".bat");
#else
        QFile jmolFile(workingDir_+"plot"+atoBaseFileName+".sh");
#endif
        if(!jmolFile.open(QFile::WriteOnly | QFile::Text))
        {
            QMessageBox msgBox;
            msgBox.setText("Could not make Jmol plotting file.");
            msgBox.exec();
            return;
        }

        QTextStream stream(&jmolFile);
#ifdef _WIN32
        stream << "set EPSRbin=" << epsrBinDir_ << "\n"
               << "set EPSRrun=" << workingDir_ << "\n"
               << "%EPSRbin%plotato.exe " << workingDir_ << " plotato " << atoBaseFileName << " 3 0 0\n";
#else
        stream << "export EPSRbin=" << epsrBinDir_ << "\n"
               << "export EPSRrun=" << workingDir_ << "\n"
               << "\"$EPSRbin\"'plotato' " << workingDir_ << " plotato " << atoBaseFileName << " 3 0 0\n";
#endif
        jmolFile.close();

        QDir::setCurrent(workingDir_);

        QProcess processplotato;
        processplotato.setProcessChannelMode(QProcess::ForwardedChannels);
#ifdef _WIN32
        processplotato.startDetached("plot"+atoBaseFileName+".bat");
#else
        processplotato.startDetached("sh plot"+atoBaseFileName+".sh");
#endif

        ui.messagesLineEdit->setText(".mol file plotted in separate window");
    }
}

void MainWindow::on_removeMolFileButton_clicked(bool checked)
{
    int row = ui.molFileList->currentRow();
    if (ui.molFileList->count() > 1)
    {
            ui.molFileList->takeItem(row);
            setSelectedMolFile();
            ui.atoFileTable->removeRow(row);
            nMolFiles = ui.molFileList->count();
    }
    else
    {
        ui.molFileList->clear();
        ui.atoFileTable->clearContents();
        ui.atoFileTable->setRowCount(0);

        ui.molAtomTable->clearContents();
        ui.molAtomTable->setRowCount(0);
        ui.molBondTable->clearContents();
        ui.molBondTable->setRowCount(0);
        ui.molAngleTable->clearContents();
        ui.molAngleTable->setRowCount(0);
        ui.molDihedralTable->clearContents();
        ui.molDihedralTable->setRowCount(0);
        ui.molRotTable->clearContents();
        ui.molRotTable->setRowCount(0);
        ui.molLJTable->clearContents();
        ui.molLJTable->setRowCount(0);
        ui.molBoxA->clear();
        ui.molBoxB->clear();
        ui.molBoxC->clear();
        ui.molBoxPhi->clear();
        ui.molBoxTheta->clear();
        ui.molBoxChi->clear();

        ui.molFileTabWidget->setEnabled(false);
        ui.viewMolFileButton->setEnabled(false);
        ui.removeMolFileButton->setEnabled(false);
        ui.updateMolFileButton->setEnabled(false);
        ui.mixatoButton->setEnabled(false);
        ui.addatoButton->setEnabled(false);
    }

    //save .pro file
    save();

    ui.messagesLineEdit->setText(".mol file removed");
}

void MainWindow::on_molFileList_itemSelectionChanged()
{
    if (ui.molFileList->count() !=0 )
    {
        setSelectedMolFile();
    }
}

bool MainWindow::readMolFile()
{
    QFile file(workingDir_+molFileName_);
    if(!file.open(QFile::ReadOnly | QFile::Text))
    {
        QMessageBox msgBox;
        msgBox.setText("Could not open .mol file");
        msgBox.exec();
        return 0;
    }
    QTextStream stream(&file);
    QString line;
    QStringList dataLine;
    dataLine.clear();

    QStringList atomLabels;
    QStringList atomTypes;
    QStringList bondAtoms;
    QStringList bondDistances;
    QStringList angleAtoms;
    QStringList bondAngles;
    QStringList dihedralAtoms;
    QStringList dihedrals;
    QStringList rotAtom1;
    QStringList rotAtom2;
    QStringList ljAtoms;
    QStringList sigmas;
    QStringList epsilons;
    QStringList masses;
    QStringList charges;
    QStringList ljTypes;
    atomLabels.clear();
    atomTypes.clear();
    bondAtoms.clear();
    bondDistances.clear();
    angleAtoms.clear();
    bondAngles.clear();
    dihedralAtoms.clear();
    dihedrals.clear();
    rotAtom1.clear();
    rotAtom2.clear();
    ljAtoms.clear();
    sigmas.clear();
    epsilons.clear();
    masses.clear();
    charges.clear();
    ljTypes.clear();

    do
    {
        line = stream.readLine();
        dataLine = line.split(" ", QString::SkipEmptyParts);
        if (dataLine.count() == 0) break;
        if (dataLine.at(0) == "changelabel")
        {
            atomLabels.append(dataLine.at(1));
            atomTypes.append(dataLine.at(2));
        }
        if (dataLine.at(0) == "bond")
        {
            bondAtoms.append(dataLine.at(1)+" "+dataLine.at(2));
            bondDistances.append(dataLine.at(3));
        }
        if (dataLine.at(0) == "angle")
        {
            angleAtoms.append(dataLine.at(1)+" "+dataLine.at(2)+" "+dataLine.at(3));
            bondAngles.append(dataLine.at(4));
        }
        if (dataLine.at(0) == "dihedral")
        {
            dihedralAtoms.append(dataLine.at(1)+" "+dataLine.at(2)+" "+dataLine.at(3)+" "+dataLine.at(4));
            dihedrals.append(dataLine.at(5));
        }
        if (dataLine.at(0) == "rot")
        {
            rotAtom1.append(dataLine.at(1));
            rotAtom2.append(dataLine.at(2));
        }
        if (dataLine.at(0) == "potential")
        {
            ljAtoms.append(dataLine.at(1));
            sigmas.append(dataLine.at(2));
            epsilons.append(dataLine.at(3));
            masses.append(dataLine.at(4));
            charges.append(dataLine.at(5));
            ljTypes.append(dataLine.at(6));
        }
//        if (dataLine.at(0) == "ecoredcore")
//        {
//            ecorestr=dataLine.at(1);
//            dcorestr=dataLine.at(2);
//        }
    } while (!line.isNull());
    file.close();

    //read .ato file for .mol to get additional details
    QString atoFile;
    atoFile = molFileName_.split(".",QString::SkipEmptyParts).at(0);
    atoFile = atoFile+".ato";

    QFile fileato(workingDir_+atoFile);
    if(!fileato.open(QFile::ReadOnly | QFile::Text))
    {
        QMessageBox msgBox;
        msgBox.setText("Could not open .ato file.");
        msgBox.exec();
        return 0;
    }
    QTextStream streamato(&fileato);
    QString lineato;
    QStringList dataLineato;
    dataLineato.clear();

    lineato = streamato.readLine();
    dataLineato = lineato.split(" ", QString::SkipEmptyParts);
    if (dataLineato.count() == 3)       //This is for cubic .ato files
    {
        double boxLength = dataLineato.at(1).toDouble();
        QString boxLengthstr;
        boxLengthstr.sprintf("%.4f", boxLength);
        ui.molBoxA->setText(boxLengthstr);
        ui.molBoxB->setText(boxLengthstr);
        ui.molBoxC->setText(boxLengthstr);
        ui.molBoxPhi->setText("90.0000");
        ui.molBoxTheta->setText("0.0000");
        ui.molBoxChi->setText("0.0000");
    }
    else                                //this is for non-cubic .ato files
    {
        lineato = streamato.readLine();
        dataLineato = lineato.split(" ", QString::SkipEmptyParts);
        double boxa = dataLineato.at(0).toDouble();
        double boxb = dataLineato.at(1).toDouble();
        double boxc = dataLineato.at(2).toDouble();
        QString boxastr;
        boxastr.sprintf("%.4f", boxa);
        QString boxbstr;
        boxbstr.sprintf("%.4f", boxb);
        QString boxcstr;
        boxcstr.sprintf("%.4f", boxc);
        ui.molBoxA->setText(boxastr);
        ui.molBoxB->setText(boxbstr);
        ui.molBoxC->setText(boxcstr);

        lineato = streamato.readLine();
        dataLineato = lineato.split(" ", QString::SkipEmptyParts);
        double boxtb = dataLineato.at(0).toDouble();
        double boxtc = dataLineato.at(1).toDouble();
        double boxpb = dataLineato.at(2).toDouble();
        QString boxtbstr;
        boxtbstr.sprintf("%.4f", boxtb);
        QString boxtcstr;
        boxtcstr.sprintf("%.4f", boxtc);
        QString boxpbstr;
        boxpbstr.sprintf("%.4f", boxpb);
        ui.molBoxPhi->setText(boxtbstr);
        ui.molBoxTheta->setText(boxtcstr);
        ui.molBoxChi->setText(boxpbstr);
    }

    lineato = streamato.readLine();
    dataLineato = lineato.split(" ", QString::SkipEmptyParts);
    QString tetherTol = dataLineato.at(0);

    lineato = streamato.readLine();
    if (lineato.contains("T0") == true)
    {
        ui.molTetherCheckBox->setChecked(true);
        dataLineato = lineato.split(" ", QString::SkipEmptyParts);
        ui.tolLineEdit->setText(tetherTol);
        QString tetherCoordX = dataLineato.at(4);
        ui.tetherCoordXLineEdit->setText(tetherCoordX);
        QString tetherCoordY = dataLineato.at(5);
        ui.tetherCoordYLineEdit->setText(tetherCoordY);
        QString tetherCoordZ = dataLineato.at(6);
        ui.tetherCoordZLineEdit->setText(tetherCoordZ);
        QString string = dataLineato.at(7);
        QString tetherAtomStr = string.remove("T");
        int tetherAtom = tetherAtomStr.toInt();
        tetherAtomStr = QString::number(tetherAtom);
        ui.tetherAtomLineEdit->setText(tetherAtomStr);
    }
    else
    {
        ui.molTetherCheckBox->setChecked(false);
        ui.tolLineEdit->clear();
        ui.tetherCoordXLineEdit->clear();
        ui.tetherCoordYLineEdit->clear();
        ui.tetherCoordZLineEdit->clear();
        ui.tetherAtomLineEdit->clear();
    }

    // TGAY 02/2018 - The ato file may not contain ecore and dcore values, so set the ui widgets to something sensible to avoid complaints later on
    ui.molEcoreLineEdit->setText("1.0");
    ui.molDcoreLineEdit->setText("0.0");
    numberComponentAtomLabels.clear();
    numberComponentAtomLabels.resize(ljAtoms.count());
    QRegExp atomLabelrx(" ([A-Z][A-Za-z0-9 ]{2})   ([0-9 ]{1,4})      0");
    QRegExp ecoredcorerx("  ([0-9]{1}[.]{1}[0-9]{1,}[E+-]{2}[0-9]{2})  ([0-9]{1}[.]{1}[0-9]{1,}[E+-]{2}[0-9]{2})");
    do {
        lineato = streamato.readLine();
        if (atomLabelrx.exactMatch(lineato))
        {
            dataLineato = lineato.split("  ", QString::SkipEmptyParts);
            QString data = dataLineato.at(0).trimmed();
            for (int i = 0; i < ljAtoms.count(); i++)
            {
                if (data == ljAtoms.at(i))
                {
                    //add 1 to label 'counter' each time find each label (therefore order is the same as ljAtoms)
                    numberComponentAtomLabels.operator [](i)++;
                }
            }
            //include way to get the number (atomLabel) for each atom even if a changelabel section isn't listed as this is useful for analyses**********************************************************
        }
        if (ecoredcorerx.exactMatch(lineato))
        {
            dataLineato = lineato.split("  ", QString::SkipEmptyParts);
            ui.molEcoreLineEdit->setText(dataLineato.at(0));
            ui.molDcoreLineEdit->setText(dataLineato.at(1));
        }
    } while (!lineato.isNull());
    fileato.close();
        
    //number of entries lists
    const int N_bondDistances = bondDistances.count();
    const int N_bondAngles = bondAngles.count();
    const int N_dihedrals = dihedrals.count();
    const int N_rotGroups = rotAtom1.count();
    const int N_atomLabels = atomLabels.count();
    const int N_LJparam = ljAtoms.count();

    //show data in Tables
    ui.molAtomTable->setRowCount(N_atomLabels);
    ui.molAtomTable->verticalHeader()->setVisible(false);
    for (int i = 0; i < N_atomLabels; ++i)
    {
        QTableWidgetItem *item = new QTableWidgetItem(atomLabels.at(i));
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        ui.molAtomTable->setItem(i,0, item);
        ui.molAtomTable->setItem(i,1, new QTableWidgetItem(atomTypes.at(i)));
    }

    ui.molBondTable->setRowCount(N_bondDistances);
    ui.molBondTable->verticalHeader()->setVisible(false);
    for (int i = 0; i < N_bondDistances; ++i)
    {
        ui.molBondTable->setItem(i,0, new QTableWidgetItem(bondAtoms.at(i)));
        ui.molBondTable->setItem(i,1, new QTableWidgetItem(bondDistances.at(i)));
    }

    ui.molAngleTable->setRowCount(N_bondAngles);
    ui.molAngleTable->verticalHeader()->setVisible(false);
    for (int i = 0; i < N_bondAngles; ++i)
    {
        ui.molAngleTable->setItem(i,0, new QTableWidgetItem(angleAtoms.at(i)));
        ui.molAngleTable->setItem(i,1, new QTableWidgetItem(bondAngles.at(i)));
    }

    ui.molDihedralTable->setRowCount(N_dihedrals);
    ui.molDihedralTable->verticalHeader()->setVisible(false);
    for (int i = 0; i < N_dihedrals; ++i)
    {
        ui.molDihedralTable->setItem(i,0, new QTableWidgetItem(dihedralAtoms.at(i)));
        ui.molDihedralTable->setItem(i,1, new QTableWidgetItem(dihedrals.at(i)));
    }

    ui.molRotTable->setRowCount(N_rotGroups);
    ui.molRotTable->verticalHeader()->setVisible(false);
    for (int i = 0; i < N_rotGroups; ++i)
    {
        ui.molRotTable->setItem(i,0, new QTableWidgetItem(rotAtom1.at(i)));
        ui.molRotTable->setItem(i,1, new QTableWidgetItem(rotAtom2.at(i)));
    }

    ui.molLJTable->setRowCount(N_LJparam);
    ui.molLJTable->verticalHeader()->setVisible(false);

    if (atomTypes.count() < 1)
    {
        //if no atomTypes are listed (ie. no changelabel section, then make ljatom uneditable
        for (int i = 0; i < N_LJparam; ++i)
        {
            QTableWidgetItem *item = new QTableWidgetItem(ljAtoms.at(i));
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            ui.molLJTable->setItem(i,0, item);
            ui.molLJTable->setItem(i,1, new QTableWidgetItem(sigmas.at(i)));
            ui.molLJTable->setItem(i,2, new QTableWidgetItem(epsilons.at(i)));
            ui.molLJTable->setItem(i,3, new QTableWidgetItem(masses.at(i)));
            ui.molLJTable->setItem(i,4, new QTableWidgetItem(charges.at(i)));
            ui.molLJTable->setItem(i,5, new QTableWidgetItem(ljTypes.at(i)));
        }
    }
    else
    {
        for (int i = 0; i < N_LJparam; ++i)
        {
            ui.molLJTable->setItem(i,0, new QTableWidgetItem(ljAtoms.at(i)));
            ui.molLJTable->setItem(i,1, new QTableWidgetItem(sigmas.at(i)));
            ui.molLJTable->setItem(i,2, new QTableWidgetItem(epsilons.at(i)));
            ui.molLJTable->setItem(i,3, new QTableWidgetItem(masses.at(i)));
            ui.molLJTable->setItem(i,4, new QTableWidgetItem(charges.at(i)));
            ui.molLJTable->setItem(i,5, new QTableWidgetItem(ljTypes.at(i)));
        }
    }


    ui.molBondTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui.molBondTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui.molAngleTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui.molAngleTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui.molDihedralTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui.molDihedralTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui.molRotTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui.molRotTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui.molLJTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui.molLJTable->setSelectionMode(QAbstractItemView::SingleSelection);

    //calculate charge for selected .mol file
    double molChargeCalcd = 0;
    //iterate over each atom in the LJ parameters (as these are present even if the changelabel section isn't)
    for (int i = 0; i < ljAtoms.count(); i++)
    {
        //get charge for atom type i
        QString atomTypeChargeStr = charges.at(i);
        double atomTypeCharge = QString(atomTypeChargeStr).toDouble();
        //get total charge contributed by atom type i by multiplying the number of i with its charge and add it to the overall charge
        molChargeCalcd = molChargeCalcd+(numberComponentAtomLabels.at(i)*atomTypeCharge);
    }
    QString molChargeCalcdStr;
    molChargeCalcdStr.sprintf("%.4f", molChargeCalcd);
    ui.molChargeLabel->setText(molChargeCalcdStr);
    QTableWidgetItem *item = new QTableWidgetItem(molChargeCalcdStr);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    int row = ui.molFileList->currentRow();
    ui.atoFileTable->setItem(row,1, item);

    return true;
}

bool MainWindow::readAtoFile()
{
    QFile file(workingDir_+molFileName_);
    if(!file.open(QFile::ReadOnly | QFile::Text))
    {
        QMessageBox msgBox;
        msgBox.setText("Could not open component .ato file");
        msgBox.exec();
        return 0;
    }
    QTextStream stream(&file);
    QString line;
    QStringList dataLine;
    dataLine.clear();

    line = stream.readLine();
    dataLine=line.split(" ",QString::SkipEmptyParts);
    if (dataLine.count() == 3)       //This is for cubic .ato files
    {
        double boxLength = dataLine.at(1).toDouble();
        QString boxLengthstr;
        boxLengthstr.sprintf("%.4f", boxLength);
        ui.molBoxA->setText(boxLengthstr);
        ui.molBoxB->clear();
        ui.molBoxC->clear();
        ui.molBoxPhi->setText("90.0000");
        ui.molBoxTheta->setText("0.0000");
        ui.molBoxChi->setText("0.0000");
    }
    else                                //this is for non-cubic .ato files
    {
        line = stream.readLine();
        dataLine = line.split("  ", QString::SkipEmptyParts);
        double boxa = dataLine.at(0).toDouble();
        double boxb = dataLine.at(1).toDouble();
        double boxc = dataLine.at(2).toDouble();
        QString boxastr;
        boxastr.sprintf("%.4f", boxa);
        QString boxbstr;
        boxbstr.sprintf("%.4f", boxb);
        QString boxcstr;
        boxcstr.sprintf("%.4f", boxc);
        ui.molBoxA->setText(boxastr);
        ui.molBoxB->setText(boxbstr);
        ui.molBoxC->setText(boxcstr);

        line = stream.readLine();
        dataLine = line.split(" ", QString::SkipEmptyParts);
        double boxtb = dataLine.at(0).toDouble();
        double boxtc = dataLine.at(1).toDouble();
        double boxpb = dataLine.at(2).toDouble();
        QString boxtbstr;
        boxtbstr.sprintf("%.4f", boxtb);
        QString boxtcstr;
        boxtcstr.sprintf("%.4f", boxtc);
        QString boxpbstr;
        boxpbstr.sprintf("%.4f", boxpb);
        ui.molBoxPhi->setText(boxtbstr);
        ui.molBoxTheta->setText(boxtcstr);
        ui.molBoxChi->setText(boxpbstr);
    }

    line = stream.readLine();
    dataLine = line.split("  ", QString::SkipEmptyParts);
    QString tetherTol = dataLine.at(0);

    line = stream.readLine();
    if (line.contains("T0") == true)
    {
        ui.molTetherCheckBox->setChecked(true);
        dataLine = line.split(" ",QString::SkipEmptyParts);
        ui.tolLineEdit->setText(tetherTol);
        ui.tetherCoordXLineEdit->setDisabled(true);
        ui.tetherCoordYLineEdit->setDisabled(true);
        ui.tetherCoordZLineEdit->setDisabled(true);
        QString string = dataLine.at(7);
        QString tetherAtomStr = string.remove("T");
        int tetherAtom = tetherAtomStr.toInt();
        tetherAtomStr = QString::number(tetherAtom);
        ui.tetherAtomLineEdit->setText(tetherAtomStr);
    }
    else
    {
        ui.molTetherCheckBox->setChecked(false);
        ui.tolLineEdit->setText("0.1");
        ui.tetherCoordXLineEdit->setText("0.000000E+00");
        ui.tetherCoordYLineEdit->setText("0.000000E+00");
        ui.tetherCoordZLineEdit->setText("0.000000E+00");
        ui.tetherAtomLineEdit->setText("0");
    }

    QStringList atomLabels;
    QStringList atomTypes;
    QStringList ljAtoms;
    QStringList sigmas;
    QStringList epsilons;
    QStringList masses;
    QStringList charges;
    QStringList ljTypes;
    atomLabels.clear();
    atomTypes.clear();
    ljAtoms.clear();
    sigmas.clear();
    epsilons.clear();
    masses.clear();
    charges.clear();
    ljTypes.clear();

    QRegExp ljAtomLabelrx(" ([A-Z][A-Za-z0-9 ]{2}) ([A-Za-z ]{1,2})   ([0-1]{1})");
    QRegExp ecoredcorerx("  ([0-9]{1}[.]{1}[0-9]{1,}[E+-]{2}[0-9]{2})  ([0-9]{1}[.]{1}[0-9]{1,}[E+-]{2}[0-9]{2})");

    do
    {
        line = stream.readLine();
        if (ljAtomLabelrx.exactMatch(line))
        {
            dataLine = line.split("  ", QString::SkipEmptyParts);
            ljAtoms.append(dataLine.at(0));
            ljTypes.append(dataLine.at(1));

            line = stream.readLine();
            dataLine = line.split(" ", QString::SkipEmptyParts);
            sigmas.append(dataLine.at(0));
            epsilons.append(dataLine.at(1));
            masses.append(dataLine.at(2));
            charges.append(dataLine.at(3));
        }
        if (ecoredcorerx.exactMatch(line))
        {
	    dataLine = line.split("  ",QString::SkipEmptyParts);
            ui.molEcoreLineEdit->setText(dataLine.at(0));
            ui.molDcoreLineEdit->setText(dataLine.at(1));
        }
    } while (!line.isNull());
    file.close();

    //re-open and stream file to get number of each ljatom present in file
    file.open(QFile::ReadOnly | QFile::Text);
    numberComponentAtomLabels.clear();
    numberComponentAtomLabels.resize(ljAtoms.count());
    QRegExp atomLabelrx(" ([A-Z][A-Za-z0-9 ]{2})   ([0-9 ]{1,4})      0");
    do {
        line = stream.readLine();
        if (atomLabelrx.exactMatch(line))
        {
            dataLine = line.split("  ", QString::SkipEmptyParts);
            atomTypes.append(dataLine.at(0));
            atomLabels.append(dataLine.at(1));
            for (int i = 0; i < ljAtoms.count(); i++)
            {
                if (dataLine.at(0) == ljAtoms.at(i))
                {
                    //add 1 to label 'counter' each time find each label (therefore order is the same as ljAtoms)
                    numberComponentAtomLabels.operator [](i)++;
                }
            }
        }
    } while (!line.isNull());
    file.close();

    //show data in Tables
    ui.molAtomTable->setRowCount(atomTypes.count());
    ui.molAtomTable->verticalHeader()->setVisible(false);
    for (int i = 0; i < atomTypes.count(); ++i)
    {
        ui.molAtomTable->setItem(i,0, new QTableWidgetItem(atomLabels.at(i)));
        ui.molAtomTable->setItem(i,1, new QTableWidgetItem(atomTypes.at(i)));
    }

    ui.molLJTable->setRowCount(ljTypes.count());
    ui.molLJTable->verticalHeader()->setVisible(false);
    for (int i = 0; i < ljTypes.count(); ++i)
    {
        QTableWidgetItem *item = new QTableWidgetItem(ljAtoms.at(i));
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        ui.molLJTable->setItem(i,0, item);
        ui.molLJTable->setItem(i,1, new QTableWidgetItem(sigmas.at(i)));
        ui.molLJTable->setItem(i,2, new QTableWidgetItem(epsilons.at(i)));
        ui.molLJTable->setItem(i,3, new QTableWidgetItem(masses.at(i)));
        ui.molLJTable->setItem(i,4, new QTableWidgetItem(charges.at(i)));
        ui.molLJTable->setItem(i,5, new QTableWidgetItem(ljTypes.at(i)));
    }

    ui.molBondTable->setRowCount(0);
    ui.molAngleTable->setRowCount(0);
    ui.molDihedralTable->setRowCount(0);
    ui.molRotTable->setRowCount(0);

    ui.molLJTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui.molLJTable->setSelectionMode(QAbstractItemView::SingleSelection);

    //calculate charge for selected .ato file
    double molChargeCalcd = 0;
    //iterate over each atom in the LJ parameters (as these are present even if the changelabel section isn't)
    for (int i = 0; i < ljAtoms.count(); i++)
    {
        //get charge for atom type i
        QString atomTypeChargeStr = charges.at(i);
        double atomTypeCharge = QString(atomTypeChargeStr).toDouble();
        //get total charge contributed by atom type i by multiplying the number of i with its charge and add it to the overall charge
        molChargeCalcd = molChargeCalcd+(numberComponentAtomLabels.at(i)*atomTypeCharge);
    }
    QString molChargeCalcdStr;
    molChargeCalcdStr.sprintf("%.4f", molChargeCalcd);
    ui.molChargeLabel->setText(molChargeCalcdStr);
    QTableWidgetItem *item = new QTableWidgetItem(molChargeCalcdStr);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    int row = ui.molFileList->currentRow();
    ui.atoFileTable->setItem(row,1, item);

    return true;
}

bool MainWindow::updateMolFile()
{
    if (ui.molFileList->count() == 0)
    {
        return false;
    }

    QRegExp rxnumbers("\\d*");
    QRegExp rxatomTypes("[A-Z][A-Za-z0-9]?[A-Za-z0-9]?");

    //check there are no empty cells and that the atom types are the same in the atom types table and LJ table - this doesn't work if the atom labels are the same as the element labels******************************************************************************************
    if (ui.molAtomTable->rowCount() > 0)
    {
        for (int i = 0; i < ui.molLJTable->rowCount(); i++)
        {
            QList<QTableWidgetItem*> atomTypeItem = ui.molAtomTable->findItems(ui.molLJTable->item(i,0)->text(), Qt::MatchExactly);
            if (atomTypeItem.isEmpty())
            {
                QMessageBox msgBox;
                msgBox.setText("Mismatch between atom types and those listed in Lennard-Jones parameters");
                msgBox.exec();
                return 0;
            }
            if (ui.molLJTable->item(i,0)->text().isEmpty() || ui.molLJTable->item(i,1)->text().isEmpty() || ui.molLJTable->item(i,2)->text().isEmpty()
                    || ui.molLJTable->item(i,3)->text().isEmpty() || ui.molLJTable->item(i,4)->text().isEmpty() || ui.molLJTable->item(i,5)->text().isEmpty())
            {
                QMessageBox msgBox;
                msgBox.setText("One of the parameters defining the Lennard Jones potential or element type is missing");
                msgBox.exec();
                return false;
            }
            if (!rxatomTypes.exactMatch(ui.molLJTable->item(i,0)->text()))
            {
                QMessageBox msgBox;
                msgBox.setText("At least one of the atom Type names is not compatible with EPSR.\nAtom Type names must be a maximum of 3 characters long, the first charcter is a capital letter and the remaining 2 optional characters are letters and/or numbers.");
                msgBox.exec();
                return false;
            }
        }

        for (int i = 0; i < ui.molAtomTable->rowCount(); i++)
        {
            QList<QTableWidgetItem*> atomTypeItem = ui.molLJTable->findItems(ui.molAtomTable->item(i,1)->text(), Qt::MatchExactly);
            if (atomTypeItem.isEmpty())
            {
                QMessageBox msgBox;
                msgBox.setText("Mismatch between atom types and those listed in Lennard-Jones parameters");
                msgBox.exec();
                return 0;
            }
        }
    }
    if (ui.molBondTable->rowCount() != 0)
    {
        for (int i = 0; i < ui.molBondTable->rowCount(); ++i)
        {
            if (ui.molBondTable->item(i,0)->text().split(" ", QString::SkipEmptyParts).count() !=2)
            {
                QMessageBox msgBox;
                msgBox.setText("The number of atoms used in the bond definition is not appropriate.");
                msgBox.exec();
                return false;
            }
            if (ui.molBondTable->item(i,1)->text().isEmpty())
            {
                QMessageBox msgBox;
                msgBox.setText("One of the values for a distance restraint is missing");
                msgBox.exec();
                return false;
            }
        }
    }
    if (ui.molAngleTable->rowCount() != 0)
    {
        for (int i = 0; i < ui.molAngleTable->rowCount(); ++i)
        {
            if (ui.molAngleTable->item(i,0)->text().split(" ", QString::SkipEmptyParts).count() !=3)
            {
                QMessageBox msgBox;
                msgBox.setText("The number of atoms used in the angle definition is not appropriate.");
                msgBox.exec();
                return false;
            }
            if (ui.molAngleTable->item(i,1)->text().isEmpty())
            {
                QMessageBox msgBox;
                msgBox.setText("One of the values for an angle restraint is missing");
                msgBox.exec();
                return false;
            }
        }
    }
    if (ui.molDihedralTable->rowCount() != 0 )
    {
        for (int i = 0; i < ui.molDihedralTable->rowCount(); ++i)
        {
            if (!rxnumbers.exactMatch(ui.molDihedralTable->item(i,0)->text().split(" ", QString::SkipEmptyParts).at(0)))
            {
                QMessageBox msgBox;
                msgBox.setText("The atom label must be used to define dihedral restriants (not the atom type).");
                msgBox.exec();
                return false;
            }
            if (ui.molDihedralTable->item(i,0)->text().split(" ", QString::SkipEmptyParts).count() !=4)
            {
                QMessageBox msgBox;
                msgBox.setText("The number of atoms used in the dihedral definition is not appropriate.");
                msgBox.exec();
                return false;
            }
            if (ui.molDihedralTable->item(i,1)->text().isEmpty())
            {
                QMessageBox msgBox;
                msgBox.setText("One of the values for a dihedral restraint is missing");
                msgBox.exec();
                return false;
            }
        }
    }
    if (ui.molRotTable->rowCount() != 0 )
    {
        for (int i = 0; i < ui.molRotTable->rowCount(); ++i)
        {
            if (!rxnumbers.exactMatch(ui.molRotTable->item(i,0)->text().split(" ", QString::SkipEmptyParts).at(0)))
            {
                QMessageBox msgBox;
                msgBox.setText("The atom label must be used to define rotational axes (not the atom type).");
                msgBox.exec();
                return false;
            }
            if (ui.molRotTable->item(i,0)->text().isEmpty())
            {
                QMessageBox msgBox;
                msgBox.setText("One of the atoms defining a rotational group is missing");
                msgBox.exec();
                return false;
            }
            if (ui.molRotTable->item(i,1)->text().isEmpty())
            {
                QMessageBox msgBox;
                msgBox.setText("One of the atoms defining a rotational group is missing");
                msgBox.exec();
                return false;
            }
        }
    }


    //check all other boxes are filled in
    if (ui.molEcoreLineEdit->text().isEmpty() || ui.molDcoreLineEdit->text().isEmpty())
    {
        QMessageBox msgBox;
        msgBox.setText("One of the parameters defining ecoredcore is missing");
        msgBox.exec();
        return false;
    }
    if (ui.molTetherCheckBox->isChecked() == true)
    {
        if (ui.tolLineEdit->text().isEmpty() || ui.tetherAtomLineEdit->text().isEmpty() || ui.tetherCoordXLineEdit->text().isEmpty()
                || ui.tetherCoordYLineEdit->text().isEmpty() || ui.tetherCoordZLineEdit->text().isEmpty())
        {
            QMessageBox msgBox;
            msgBox.setText("One of the parameters defining the tethering is missing.");
            msgBox.exec();
            return false;
        }
    }


    QDir::setCurrent(workingDir_);

    QFile fileRead(workingDir_+molFileName_);
    QFile fileWrite(workingDir_+"temp.txt");

    if(!fileRead.open(QFile::ReadOnly | QFile::Text))
    {
        QMessageBox msgBox;
        msgBox.setText("Could not open .mol file");
        msgBox.exec();
        return false;
    }
    if(!fileWrite.open(QFile::WriteOnly | QFile::Text))
    {
        QMessageBox msgBox;
        msgBox.setText("Could not open temporary file");
        msgBox.exec();
        return false;
    }

    QTextStream streamRead(&fileRead);
    QTextStream streamWrite(&fileWrite);
    QString line;
    QStringList dataLine;
    dataLine.clear();
    QString original;

    while (!streamRead.atEnd())
    {
        line = streamRead.readLine();
        if(!line.contains("bond")&&!line.contains("angle")
                &&!line.contains("dihedral")&&!line.contains("rot")
                &&!line.contains("changelabel")&&!line.contains("potential")
                &&!line.contains("temperature")&&!line.contains("vibtemp")
                &&!line.contains("angtemp")&&!line.contains("dihtemp")
                &&!line.contains("density")&&!line.contains("ecoredcore"))
        {
            original.append(line+"\n");
        }
    }
    fileWrite.resize(0);
    streamWrite << original;
    fileRead.close();

    if (ui.molBondTable->rowCount() != 0)
    {
        for (int i = 0; i < ui.molBondTable->rowCount(); ++i)
        {
            QString atoms = ui.molBondTable->item(i,0)->text();
            QString atom1 = atoms.split(" ", QString::SkipEmptyParts).at(0);
            QString atom2 = atoms.split(" ", QString::SkipEmptyParts).at(1);
            if (rxnumbers.exactMatch(atom1))
            {
                atom1 = atom1.rightJustified(3, '0');
                atom2 = atom2.rightJustified(3, '0');
            }
            streamWrite << "bond " << atom1 << " " << atom2 << "    " << ui.molBondTable->item(i,1)->text() << "\n";
        }
    }

    if (ui.molAngleTable->rowCount() != 0)
    {
        for (int i = 0; i < ui.molAngleTable->rowCount(); ++i)
        {
            QString atoms = ui.molAngleTable->item(i,0)->text();
            QString atom1 = atoms.split(" ", QString::SkipEmptyParts).at(0);
            QString atom2 = atoms.split(" ", QString::SkipEmptyParts).at(1);
            QString atom3 = atoms.split(" ", QString::SkipEmptyParts).at(2);
            if (rxnumbers.exactMatch(atom1))
            {
                atom1 = atom1.rightJustified(3, '0');
                atom2 = atom2.rightJustified(3, '0');
                atom3 = atom3.rightJustified(3, '0');
            }
            streamWrite << "angle " << atom1 << " " << atom2 << " " << atom3 << "  " << ui.molAngleTable->item(i,1)->text() << "\n";
        }
    }

    if (ui.molDihedralTable->rowCount() != 0 )
    {
        for (int i = 0; i < ui.molDihedralTable->rowCount(); ++i)
        {
            QString atoms = ui.molDihedralTable->item(i,0)->text();
            QString atom1 = atoms.split(" ", QString::SkipEmptyParts).at(0);
            QString atom2 = atoms.split(" ", QString::SkipEmptyParts).at(1);
            QString atom3 = atoms.split(" ", QString::SkipEmptyParts).at(2);
            QString atom4 = atoms.split(" ", QString::SkipEmptyParts).at(3);
            int atom1num = atom1.toInt();
            int atom2num = atom2.toInt();
            int atom3num = atom3.toInt();
            int atom4num = atom4.toInt();
            streamWrite << "dihedral  " << QString::number(atom1num) << " " << QString::number(atom2num) << " " << QString::number(atom3num) << " " << QString::number(atom4num) << "  " << ui.molDihedralTable->item(i,1)->text() << "\n";
        }
    }

    if (ui.molRotTable->rowCount() != 0 )
    {
        for (int i = 0; i < ui.molRotTable->rowCount(); ++i)
        {
            QString atom1 = ui.molRotTable->item(i,0)->text();
            QString atom2 = ui.molRotTable->item(i,1)->text();
            int atom1num = atom1.toInt();
            int atom2num = atom2.toInt();
            streamWrite << "rot  " << QString::number(atom1num) << "  " << QString::number(atom2num) << "\n";
        }
    }

    for (int i = 0; i < ui.molAtomTable->rowCount(); ++i)
    {
        QString atomNumber = QString("%1").arg(i+1, 3, 10, QChar('0'));
        streamWrite << "changelabel " << atomNumber << " " << ui.molAtomTable->item(i,1)->text() << "\n";
    }

    for (int i = 0; i < ui.molLJTable->rowCount(); ++i)
    {
        //need to pad atom label with a blank space at end (item 0) and charges with a blank space if '-' not present (item 4)
        streamWrite << "potential " << ui.molLJTable->item(i,0)->text() << "  " << ui.molLJTable->item(i,1)->text() << "  " << ui.molLJTable->item(i,2)->text() << "  " << ui.molLJTable->item(i,3)->text() << " " << ui.molLJTable->item(i,4)->text() << " " << ui.molLJTable->item(i,5)->text() << "\n";
    }
    streamWrite << "temperature  0.300000E+03\n"
              "vibtemp  0.650000E+02\n"
              "angtemp  0.100000E+01\n"
              "dihtemp  0.100000E+01\n";

    QString ecorestr = ui.molEcoreLineEdit->text();
    double ecore = ecorestr.toDouble();
    ecorestr = QString("%1").arg(ecore, 0, 'E', 5);
    QString dcorestr = ui.molDcoreLineEdit->text();
    double dcore = dcorestr.toDouble();
    dcorestr = QString("%1").arg(dcore, 0, 'E', 5);

    streamWrite << "ecoredcore    " << ecorestr << "    " << dcorestr << "\n";
    streamWrite << "density  0.100000E-02\n";

    fileWrite.close();

    //rename temp file as .mol file to copy over changes and delete temp file
    fileRead.remove();
    fileWrite.rename(workingDir_+molFileName_);
    messageText_ += "finished updating mol file "+molFileName_+"\n";

    //run fmole 0 times to make sure everything is consistent and update ato with dihedrals etc
    QDir::setCurrent(workingDir_);

    QString molBaseFileName = molFileName_.split(".",QString::SkipEmptyParts).at(0);

#ifdef _WIN32
    processEPSR_.start(epsrBinDir_+"fmole.exe", QStringList() << workingDir_ << "fmole" << molBaseFileName << "0" << "0");
#else
    processEPSR_.start(epsrBinDir_+"fmole", QStringList() << workingDir_ << "fmole" << molBaseFileName << "0" << "0");
#endif
    if (!processEPSR_.waitForStarted()) return false;

    if (!processEPSR_.waitForFinished()) return false;

    //update molecule .ato file with additional details (not those already changed via .mol file)
    QString atoFile;
    atoFile = molFileName_.split(".",QString::SkipEmptyParts).at(0);
    atoFile = atoFile+".ato";

    QFile fileato(workingDir_+atoFile);
    if(!fileato.open(QFile::ReadWrite | QFile::Text))
    {
        QMessageBox msgBox;
        msgBox.setText("Could not open .ato file");
        msgBox.exec();
        return 0;
    }
    QFile filetemp(workingDir_+"temp");
    if(!filetemp.open(QFile::ReadWrite | QFile::Text))
    {
        QMessageBox msgBox;
        msgBox.setText("Could not open temporary file");
        msgBox.exec();
        return 0;
    }
    QTextStream streamatoR(&fileato);
    QTextStream streamatoW(&filetemp);
    QString lineato;
    QStringList dataLineato;
    dataLineato.clear();
    QString originalato;

    //use default values assuming not tethered
    QString tether = "F";
    QString tetherAtomStr = "     ";
    QString tetherTol = "0.00000E+00";
    QString tetherCoordX = "0.00000E+00";
    QString tetherCoordY = "0.00000E+00";
    QString tetherCoordZ = "0.00000E+00";

    //if tether check box is ticked use these values
    if (ui.molTetherCheckBox->isChecked() == true)
    {
        tether = "T";
        tetherTol = ui.tolLineEdit->text();
        double tetherTold = tetherTol.toDouble();
        tetherTol = QString("%1").arg(tetherTold, 0, 'E', 5);
        int tetherAtom = ui.tetherAtomLineEdit->text().toInt(); //if an integer  isn't typed in the box (i.e. letters or a decimal), this process assumes a 0
        tetherAtomStr = QString("%1").arg(tetherAtom, 5, 10, QChar('0'));
        tetherCoordX = ui.tetherCoordXLineEdit->text();
        double tetherCoordXd = tetherCoordX.toDouble();
        tetherTol = QString("%1").arg(tetherCoordXd, 0, 'E', 5);
        tetherCoordY = ui.tetherCoordYLineEdit->text();
        double tetherCoordYd = tetherCoordY.toDouble();
        tetherTol = QString("%1").arg(tetherCoordYd, 0, 'E', 5);
        tetherCoordZ = ui.tetherCoordZLineEdit->text();
        double tetherCoordZd = tetherCoordZ.toDouble();
        tetherTol = QString("%1").arg(tetherCoordZd, 0, 'E', 5);
    }

    QRegExp ecoredcorerx("  ([0-9]{1}[.]{1}[0-9]{1,}[E+-]{2}[0-9]{2})  ([0-9]{1}[.]{1}[0-9]{1,}[E+-]{2}[0-9]{2})");

    lineato = streamatoR.readLine();
    dataLineato=lineato.split(" ",QString::SkipEmptyParts);
    if (dataLineato.count() == 2)       //this is for non-cubic .ato files
    {
        streamatoW << lineato << "\n";      //cell parameters cannot be changed
        lineato = streamatoR.readLine();
        streamatoW << lineato << "\n";
        lineato = streamatoR.readLine();
        streamatoW << lineato << "\n";
    }
    else                                //this is for cubic .ato files
    {
        streamatoW << lineato << "\n";     //cell parameters cannot be changed
    }
    lineato = streamatoR.readLine();
    dataLineato = lineato.split(" ",QString::SkipEmptyParts);
    streamatoW << "   " << tetherTol;  //This is the line starting with tol and contains vibtemp etc
    for (int i = 1; i < dataLineato.count(); i++ )
    {
        streamatoW << "  " << dataLineato.at(i);
    }
    streamatoW << "\n";
    lineato = streamatoR.readLine();
    dataLineato = lineato.split(" ",QString::SkipEmptyParts);
    for (int i = 0; i < 4; i++ )
    {
        streamatoW << "   " << dataLineato.at(i);   //this is the line starting with the number of atoms and containing tether
    }
    streamatoW << "   " << tetherCoordX << "   " << tetherCoordY << "   " << tetherCoordZ << "   " << tether+tetherAtomStr << "               0     1\n";

    do {
        lineato = streamatoR.readLine();
        originalato.append(lineato+"\n");
        dataLineato = lineato.split(" ",QString::SkipEmptyParts);
        if (dataLineato.count() == 2 && ecoredcorerx.exactMatch(lineato))
        {
            originalato.remove(originalato.count()-27,originalato.count());
            originalato.append("  "+ecorestr+"  "+dcorestr+"\n");
        }
        if (lineato.contains("moltype01"))
        {
            originalato.remove(lineato+"\n");
            originalato.append("   1 "+atoFile+"  0.000000E+00  0.000000E+00  0.100000E+01  0.100000E+00\n");
        }
    } while (!lineato.isNull());
    fileato.close();

    filetemp.resize(0);
    streamatoW << originalato;
    filetemp.close();

    fileato.remove();
    filetemp.rename(workingDir_+atoFile);

    //read mol file back into tables and update charge on mol file
    readMolFile();

    //if boxAtoFile exist, update box charge too
    if (!atoFileName_.isEmpty())
    {
        checkBoxCharge();
    }

    messageText_ += "\nfinished updating molecule ato file\n";
    messagesDialog.refreshMessages();
    ui.messagesLineEdit->setText(".mol file updated");

    return true;
}

bool MainWindow::updateAtoFile()
{
    if (ui.molFileList->count() == 0)
    {
        return false;
    }

    QDir::setCurrent(workingDir_);

    QFile fileRead(workingDir_+molFileName_);
    QFile fileWrite(workingDir_+"temp.txt");

    if(!fileRead.open(QFile::ReadOnly | QFile::Text))
    {
        QMessageBox msgBox;
        msgBox.setText("Could not open .ato file");
        msgBox.exec();
        return false;
    }
    if(!fileWrite.open(QFile::WriteOnly | QFile::Text))
    {
        QMessageBox msgBox;
        msgBox.setText("Could not open temporary file");
        msgBox.exec();
        return false;
    }

    QTextStream streamR(&fileRead);
    QTextStream streamW(&fileWrite);
    QString line;
    QStringList dataLine;
    dataLine.clear();
    QString original;
    QString ecorestr = ui.molEcoreLineEdit->text();
    QString dcorestr = ui.molDcoreLineEdit->text();

    QRegExp atomLabelrx(" ([A-Z][A-Za-z0-9 ]{2}) ([A-Za-z ]{1,2})   ([0-1]{1})");
    QRegExp ecoredcorerx("  ([0-9]{1}[.]{1}[0-9]{1,}[E+-]{2}[0-9]{2})  ([0-9]{1}[.]{1}[0-9]{1,}[E+-]{2}[0-9]{2})");

    line = streamR.readLine();
    dataLine=line.split(" ",QString::SkipEmptyParts);
    if (dataLine.count() == 2)       //this is for non-cubic .ato files
    {
        streamW << line << "\n";      //cell parameters cannot be changed
        line = streamR.readLine();
        streamW << line << "\n";
        line = streamR.readLine();
        streamW << line << "\n";
    }
    else                                //this is for cubic .ato files
    {
        streamW << line << "\n";     //cell parameters cannot be changed
    }

    line = streamR.readLine();
    dataLine = line.split(" ",QString::SkipEmptyParts);
    streamW << "   " << ui.tolLineEdit->text();  //This is the line starting with tol and contains vibtemp etc
    for (int i = 1; i < dataLine.count(); i++ )
    {
        streamW << "  " << dataLine.at(i);
    }
    streamW << "\n";

    int atomctr = 0;
    do
    {
        line = streamR.readLine();
        dataLine = line.split(" ",QString::SkipEmptyParts);
        if (dataLine.count() > 9)
        {
            if (ui.molTetherCheckBox->isChecked() == true && line.contains("F") == true)
            {
                line = line.replace(91, 6, "T00000");
            }
            if (ui.molTetherCheckBox->isChecked() == false && line.contains("T0") == true)
            {
                line = line.replace(91, 6, "F     ");
            }
        }
        original.append(line+"\n");
        if (atomLabelrx.exactMatch(line))
        {
            line = streamR.readLine();
            original.remove(line+"\n");
            original.append("  "+ui.molLJTable->item(atomctr,1)->text()+"  "+ui.molLJTable->item(atomctr,2)->text()+"  "+ui.molLJTable->item(atomctr,3)->text()+" "+ui.molLJTable->item(atomctr,4)->text()+"  0.00000E+00\n");
            atomctr++;
        }
        if (ecoredcorerx.exactMatch(line))
        {
            original.remove(original.count()-27,original.count());
            original.append("  "+ecorestr+"  "+dcorestr+"\n");
        }
    }while(!line.isNull());
    fileRead.close();

    fileWrite.resize(0);
    streamW << original;
    fileWrite.close();

    fileRead.remove();
    fileWrite.rename(workingDir_+molFileName_);

    //read mol file back into tables and update charge on mol file
    readAtoFile();

    //if boxAtoFile exist, apply changes to box ato file and update box charge
    if (!atoFileName_.isEmpty())
    {
        QFile fileBARead(workingDir_+atoFileName_);
        QFile fileBAWrite(workingDir_+"temp.txt");

        if(!fileBARead.open(QFile::ReadOnly | QFile::Text))
        {
            QMessageBox msgBox;
            msgBox.setText("Could not open box .ato file");
            msgBox.exec();
            return false;
        }
        if(!fileBAWrite.open(QFile::WriteOnly | QFile::Text))
        {
            QMessageBox msgBox;
            msgBox.setText("Could not open temporary file");
            msgBox.exec();
            return false;
        }

        QTextStream streamBAR(&fileBARead);
        QTextStream streamBAW(&fileBAWrite);
        QString lineBA;
        QStringList dataLineBA;
        dataLineBA.clear();
        QString originalBA;
        QRegExp atomLabelrx(" ([A-Z][A-Za-z0-9 ]{2}) ([A-Za-z ]{1,2})   ([0-1]{1})");

        do
        {
            lineBA = streamBAR.readLine();
            originalBA.append(lineBA+"\n");
            if (atomLabelrx.exactMatch(lineBA))
            {
                for (int i = 0; i < ui.molLJTable->rowCount(); i++)
                {
                    if (lineBA.contains(ui.molLJTable->item(i,0)->text()))
                    {
                        lineBA = streamBAR.readLine();
                        originalBA.append("  "+ui.molLJTable->item(i,1)->text()+"  "+ui.molLJTable->item(i,2)->text()+"  "+ui.molLJTable->item(i,3)->text()+" "+ui.molLJTable->item(i,4)->text()+"  0.00000E+00\n");
                    }
                }
            }
        } while (!lineBA.isNull());
        fileBARead.close();

        fileBAWrite.resize(0);
        streamBAW << originalBA;
        fileBAWrite.close();

        fileBARead.remove();
        fileBAWrite.rename(workingDir_+atoFileName_);

        checkBoxCharge();
    }

    messageText_ += "\nfinished updating component .ato file\n";
    messagesDialog.refreshMessages();
    ui.messagesLineEdit->setText("Component .ato file updated");

    return true;
}

void MainWindow::on_addLJRowAboveButton_clicked(bool checked)
{
    int row = 0;
    if (ui.molLJTable->rowCount() != 0 )
    {
       row = ui.molLJTable->currentRow();
    }
    ui.molLJTable->insertRow(row);
}

void MainWindow::on_addLJRowBelowButton_clicked (bool checked)
{
    int row = 0;
    if (ui.molLJTable->rowCount() != 0 )
    {
       row = ui.molLJTable->currentRow()+1;
    }
    ui.molLJTable->insertRow(row);
}

void MainWindow::on_deleteLJRowButton_clicked (bool checked)
{
    int row = ui.molLJTable->currentRow();
    ui.molLJTable->removeRow(row);
}

void MainWindow::on_addDistRowAboveButton_clicked(bool checked)
{
    int row = 0;
    if (ui.molBondTable->rowCount() != 0 )
    {
       row = ui.molBondTable->currentRow();
    }
    ui.molBondTable->insertRow(row);
}

void MainWindow::on_addDistRowBelowButton_clicked (bool checked)
{
    int row = 0;
    if (ui.molBondTable->rowCount() != 0 )
    {
       row = ui.molBondTable->currentRow()+1;
    }
    ui.molBondTable->insertRow(row);
}

void MainWindow::on_deleteDistRowButton_clicked (bool checked)
{
    int row = ui.molBondTable->currentRow();
    ui.molBondTable->removeRow(row);
}

void MainWindow::on_addAngRowAboveButton_clicked(bool checked)
{
    int row = 0;
    if (ui.molAngleTable->rowCount() != 0 )
    {
       row = ui.molAngleTable->currentRow();
    }
    ui.molAngleTable->insertRow(row);
}

void MainWindow::on_addAngRowBelowButton_clicked (bool checked)
{
    int row = 0;
    if (ui.molAngleTable->rowCount() != 0 )
    {
       row = ui.molAngleTable->currentRow()+1;
    }
    ui.molAngleTable->insertRow(row);
}

void MainWindow::on_deleteAngRowButton_clicked (bool checked)
{
    int row = ui.molAngleTable->currentRow();
    ui.molAngleTable->removeRow(row);
}

void MainWindow::on_addDihRowAboveButton_clicked(bool checked)
{
    int row = 0;
    if (ui.molDihedralTable->rowCount() != 0 )
    {
       row = ui.molDihedralTable->currentRow();
    }
    ui.molDihedralTable->insertRow(row);
}

void MainWindow::on_addDihRowBelowButton_clicked (bool checked)
{
    int row = 0;
    if (ui.molDihedralTable->rowCount() != 0 )
    {
       row = ui.molDihedralTable->currentRow()+1;
    }
    ui.molDihedralTable->insertRow(row);
}

void MainWindow::on_deleteDihRowButton_clicked (bool checked)
{
    int row = ui.molDihedralTable->currentRow();
    ui.molDihedralTable->removeRow(row);
}

void MainWindow::on_deleteDihAllButton_clicked (bool checked)
{
    ui.molDihedralTable->setRowCount(0);
}

void MainWindow::on_addRotRowAboveButton_clicked(bool checked)
{
    int row = 0;
    if (ui.molRotTable->rowCount() != 0 )
    {
       row = ui.molRotTable->currentRow();
    }
    ui.molRotTable->insertRow(row);
}

void MainWindow::on_addRotRowBelowButton_clicked (bool checked)
{
    int row = 0;
    if (ui.molRotTable->rowCount() != 0 )
    {
       row = ui.molRotTable->currentRow()+1;
    }
    ui.molRotTable->insertRow(row);
}

void MainWindow::on_deleteRotRowButton_clicked (bool checked)
{
    int row = ui.molRotTable->currentRow();
    ui.molRotTable->removeRow(row);
}

void MainWindow::on_deleteRotAllButton_clicked (bool checked)
{
    ui.molRotTable->setRowCount(0);
}

void MainWindow::on_updateMolFileButton_clicked(bool checked)
{
    if (molChangeatoFinishedTimerId_ != -1) killTimer(molChangeatoFinishedTimerId_);
    molChangeatoFinishedTimerId_ = -1;

    QString extension = molFileName_.split(".",QString::SkipEmptyParts).at(1);
    if (extension == "ato")
    {
        updateAtoFile();
    }
    else
    {
        updateMolFile();
    }
}

void MainWindow::on_molChangeAtobutton_clicked(bool checked)
{
    QMessageBox::StandardButton msgBox;
    msgBox  = QMessageBox::question(this, "Warning", "This will run changeato for the component in a terminal window.\nProceed?", QMessageBox::Ok|QMessageBox::Cancel);
    if (msgBox == QMessageBox::Cancel)
    {
        return;
    }
    else
    {
        QDir::setCurrent(workingDir_);

        QString atoBaseFileName = molFileName_.split(".",QString::SkipEmptyParts).at(0);

        QProcess processEditAto;
        processEditAto.setProcessChannelMode(QProcess::ForwardedChannels);
    #ifdef _WIN32
        processEditAto.startDetached(epsrBinDir_+"changeato.exe", QStringList() << workingDir_ << "changeato" << atoBaseFileName);
    #else
        processEditAto.startDetached(epsrBinDir_+"changeato", QStringList() << workingDir_ << "changeato" << atoBaseFileName);
    #endif
        ui.messagesLineEdit->setText("Changeato opened in separate window");

        molChangeatoFinishedTimerId_ = startTimer(2000);
        atoLastMod_ = QDateTime::currentDateTime();
    }
}

void MainWindow::on_molFmoleButton_clicked(bool checked)
{
    int fmoleIter = ui.molFmoleLineEdit->text().toInt();

    QDir::setCurrent(workingDir_);

    QString molBaseFileName = molFileName_.split(".",QString::SkipEmptyParts).at(0);

#ifdef _WIN32
    processEPSR_.start(epsrBinDir_+"fmole.exe", QStringList() << workingDir_ << "fmole" << molBaseFileName << qPrintable(QString::number(fmoleIter)));
#else
    processEPSR_.start(epsrBinDir_+"fmole", QStringList() << workingDir_ << "fmole" << molBaseFileName << qPrintable(QString::number(fmoleIter)));
#endif
    if (!processEPSR_.waitForStarted()) return;
    if (!processEPSR_.waitForFinished(1800000)) return;

    setSelectedMolFile();

    messageText_ += "\nfmole finished running for component .ato file\n";
    messagesDialog.refreshMessages();
    ui.messagesLineEdit->setText("fmole finished running for component .ato file");

}

void MainWindow::on_dockatoButton_clicked(bool checked)
{
    QMessageBox msgBox1;
    msgBox1.setText("This is not currently implemented. Use EPSRshell to create the component.");
    msgBox1.exec();
    return;

    QMessageBox::StandardButton msgBox;
    msgBox  = QMessageBox::question(this, "Warning", "This will run dockato in a terminal window.\nProceed?", QMessageBox::Ok|QMessageBox::Cancel);
    if (msgBox == QMessageBox::Cancel)
    {
        return;
    }
    else
    {
        QDir::setCurrent(workingDir_);

        QProcess processDockato;
        processDockato.setProcessChannelMode(QProcess::ForwardedChannels);
    #ifdef _WIN32
        processDockato.startDetached(epsrBinDir_+"dockato.exe", QStringList() << workingDir_ << "dockato");
    #else
        processDockato.startDetached(epsrBinDir_+"dockato", QStringList() << workingDir_ << "dockato");
    #endif
    }
}
