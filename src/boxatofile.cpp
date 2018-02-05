#include <QtGui>
#include <QPushButton>
#include <QDebug>
#include <QProcess>
#include <QString>
#include <QFile>
#include <QVector>

#include "epsrproject.h"
#include "molfiles.h"
#include "moloptionsdialog.h"
#include "boxcompositiondialog.h"
#include "addatodialog.h"
#include "removecomponentdialog.h"
#include "makemollattdialog.h"

void MainWindow::on_mixatoButton_clicked(bool checked)
{
    for (int i = 0; i < nMolFiles; i++)
    {
        if (ui.atoFileTable->item(i,2)->text().isEmpty() || ui.atoFileTable->item(i,2)->text() == "0" || ui.atoFileTable->item(i,2)->text().contains("."))
        {
            QMessageBox msgBox;
            msgBox.setText("The number of one of the components to be included is missing or zero or not an integer value");
            msgBox.exec();
            return;
        }
    }
    if (ui.numberDensityLineEdit->text().isEmpty())
    {
        QMessageBox::warning(this, tr("Atomic number density is blank"),
            tr("Input an atomic number density for the simulation box."));
    }
    else
    {
        QString atoFileBaseName = projectName_+"box";
        atoFileName_ = atoFileBaseName+".ato";

        //make a list of all the .ato files in the working directory in alphabetical order and make
        //a list of the indexes of each of the .ato files want to mix
        QDir::setCurrent(workingDir_);

        QDir dir;
        QStringList atoFilter;
        atoFilter << "*.ato";
        QStringList atoFiles = dir.entryList(atoFilter, QDir::Files, QDir::Name|QDir::IgnoreCase);
        QStringList atoFileIndexes;
        QStringList numberOfMolecules;
        for (int i = 0; i < nMolFiles; i++)
        {
            QString atoFileName = ui.atoFileTable->item(i,0)->text();
            QString atoFileIndex = QString::number(atoFiles.indexOf(atoFileName));
            if (atoFileIndex == "-1")
            {
                QMessageBox msgBox;
                msgBox.setText("One of the .ato files could not be found in the working directory.\n Check that all the .ato files listed are present in the working directory and have the same case filename as the .mol files.");
                msgBox.exec();
                return;
            }
            atoFileIndexes.append(atoFileIndex);
            QString numberMol = ui.atoFileTable->item(i,2)->text();
            numberOfMolecules.append(numberMol);
        }

        int nIndex = atoFileIndexes.count();

        QString projDir = workingDir_;
        projDir = QDir::toNativeSeparators(projDir);
    #ifdef _WIN32
        processEPSR_.start(epsrBinDir_+"mixato.exe", QStringList() << projDir << "mixato");
    #else
        processEPSR_.start(epsrBinDir_+"mixato", QStringList() << projDir << "mixato");
    #endif
        if (!processEPSR_.waitForStarted()) return;

        processEPSR_.write(qPrintable(QString::number(nIndex)+"\n"));

        for (int i = 0 ; i < nMolFiles; i++)
        {
            int newlines = atoFileIndexes.at(i).toInt();
            for (int nl = 0; nl < newlines; nl++)
            {
                processEPSR_.write("\n");
            }
            processEPSR_.write("y\n");

            int nMols = numberOfMolecules.at(i).toInt();
            processEPSR_.write(qPrintable(QString::number(nMols)+"\n"));
        }
            double numberDensity = ui.numberDensityLineEdit->text().toDouble();
            processEPSR_.write(qPrintable(QString::number(numberDensity)+"\n"));

            processEPSR_.write(qPrintable(atoFileBaseName+"\n"));

        if (!processEPSR_.waitForFinished(1800000)) return;

	if (readAtoFileBoxDetails() == false)
        {
            atoFileName_.clear();
            ui.boxAtoLabel->clear();
            ui.boxAtoCharge->clear();
            ui.boxAtoMols->clear();
            ui.boxAtoAtoms->clear();
            ui.boxAtoLengthA->clear();
            ui.boxAtoLengthB->clear();
            ui.boxAtoLengthC->clear();
            ui.boxAtoAxisA->clear();
            ui.boxAtoAxisB->clear();
            ui.boxAtoAxisG->clear();
            ui.boxAtoVol->clear();
            ui.temperatureLineEdit->clear();
            ui.vibtempLineEdit->clear();
            ui.angtempLineEdit->clear();
            ui.dihtempLineEdit->clear();
            ui.atoTetherTable->clearContents();
            ui.atoTetherTable->setRowCount(0);
            atoFileName_.clear();
            baseFileName_.clear();
            ui.ecoreLineEdit->clear();
            ui.dcoreLineEdit->clear();
            return;
        }

        QStringList testAtoAtomTypes = atoAtomTypes;
        if (testAtoAtomTypes.removeDuplicates() >= 1)
        {
            ui.boxAtoLabel->clear();
            ui.boxAtoCharge->clear();
            ui.boxAtoMols->clear();
            ui.boxAtoAtoms->clear();
            ui.boxAtoLengthA->clear();
            ui.boxAtoLengthB->clear();
            ui.boxAtoLengthC->clear();
            ui.boxAtoAxisA->clear();
            ui.boxAtoAxisB->clear();
            ui.boxAtoAxisG->clear();
            ui.boxAtoVol->clear();
            ui.temperatureLineEdit->clear();
            ui.vibtempLineEdit->clear();
            ui.angtempLineEdit->clear();
            ui.dihtempLineEdit->clear();
            ui.atoTetherTable->clearContents();
            ui.atoTetherTable->setRowCount(0);
            atoFileName_.clear();
            baseFileName_.clear();
            ui.ecoreLineEdit->clear();
            ui.dcoreLineEdit->clear();

            QMessageBox msgBox;
            msgBox.setText("The box contains duplicate atom types.\n Check that all the components have unique atom types.");
            msgBox.exec();
            return;
        }
        checkBoxCharge();
        ui.boxAtoLabel->setText(atoFileName_);
        ui.randomiseButton->setEnabled(true);
        ui.plotBoxAct->setEnabled(true);
        ui.boxCompositionButton->setEnabled(true);
        ui.updateAtoFileButton->setEnabled(true);
        ui.fmoleButton->setEnabled(true);
        ui.atoEPSRButton->setEnabled(true);
        ui.reloadBoxButton->setEnabled(true);
        ui.removeComponentButton->setEnabled(true);
        ui.dataFileBrowseButton->setEnabled(true);
        ui.removeDataFileButton->setEnabled(true);
        ui.deleteBoxAtoFileAct->setEnabled(true);

        messageText_ += "\nfinished writing "+atoFileName_+" file\n";
        messagesDialog.refreshMessages();
        ui.messagesLineEdit->setText("Finished writing box .ato file");

        //save .pro file
        save();

        jmolFile_.removePath(workingDir_);
        dir.setPath(workingDir_);
        QStringList jmolFilter;
        jmolFilter << "*.jmol";
        QStringList jmolFiles = dir.entryList(jmolFilter, QDir::Files);
        foreach (QString jmolFile, jmolFiles)
        {
            jmolFile_.removePath(jmolFile);
        }
    }
}

void MainWindow::on_addatoButton_clicked(bool checked)
{
    //*****************
    //NOTE
    //There are limitations to what will be read during the process - see addato.f for details
    //the atomic number density is calculated after addato as the size of the container determines the size of the final box.
    //the atomic overlap during addato is determined by sigma and epsilon.
    //tethering of molecules is also important prior to pressing addato.

    AddAtoDialog addAtoDialog(this);

    addAtoDialog.setModal(true);
    addAtoDialog.show();
    addAtoDialog.raise();
    addAtoDialog.activateWindow();

    atoaddDialog = addAtoDialog.exec();

    if (atoaddDialog == AddAtoDialog::Accepted)
    {
        //in case Addato doesn't work, make a copy of the simulation box ato file first
        QFile::copy(workingDir_+atoFileName_, workingDir_+atoFileName_+".copy");

        QString container = addAtoDialog.getContainer();
        QStringList atoFilesToAdd = addAtoDialog.getAtoFiles();
        QStringList numberOfMolecules = addAtoDialog.getNumberMols();

        QString atoFileBaseName = projectName_+"box";
        atoFileName_ = atoFileBaseName+".ato";

        //make a list of all the .ato files in the working directory in alphabetical order and make
        //a list of the indexes of each of the .ato files want to mix
        QDir::setCurrent(workingDir_);

        QDir dir;
        QStringList atoFilter;
        atoFilter << "*.ato";
        QStringList atoFiles = dir.entryList(atoFilter, QDir::Files, QDir::Name|QDir::IgnoreCase);
        QStringList atoFileIndexes;
        atoFileIndexes.clear();

        int containerIndex = atoFiles.indexOf(container);
        if (containerIndex == -1)
        {
            QMessageBox msgBox;
            msgBox.setText("The container .ato file could not be found in the working directory.\n Check that all the .ato files listed are present in the working directory.");
            msgBox.exec();
            return;
        }

        for (int i = 0; i < atoFilesToAdd.count(); i++)
        {
            QString atoFileName = atoFilesToAdd.at(i);
            QString atoFileIndex = QString::number(atoFiles.indexOf(atoFileName));
            if (atoFileIndex == "-1")
            {
                QMessageBox msgBox;
                msgBox.setText("One of the .ato files could not be found in the working directory.\n Check that all the .ato files listed are present in the working directory.");
                msgBox.exec();
                return;
            }
            atoFileIndexes.append(atoFileIndex);
        }

        int nIndex = atoFilesToAdd.count(); //this is the number of files to be added to the container

        QString projDir = workingDir_;
        projDir = QDir::toNativeSeparators(projDir);
#ifdef _WIN32
        processEPSR_.start(epsrBinDir_+"addato.exe", QStringList() << projDir << "addato");
#else
        processEPSR_.start(epsrBinDir_+"addato", QStringList() << projDir << "addato");
#endif
        if (!processEPSR_.waitForStarted()) return;

        processEPSR_.write(qPrintable(QString::number(nIndex)+"\n"));

        // press enter to get to each ato file listed in the table that will be added to the container
        for (int i = 0 ; i < atoFilesToAdd.count(); i++)
        {
            int newlines = atoFileIndexes.at(i).toInt();
            for (int nl = 0; nl < newlines; nl++)
            {
                processEPSR_.write("\n");
            }
            processEPSR_.write("y\n");

            processEPSR_.write(qPrintable(numberOfMolecules.at(i)+"\n"));
        }

        // press enter to get to the ato file that is the container
        for (int nl = 0; nl < containerIndex; nl++)
        {
            processEPSR_.write("\n");
        }
        processEPSR_.write("y\n");

        processEPSR_.write(qPrintable(atoFileBaseName+"\n"));

        if (!processEPSR_.waitForFinished(1800000)) return;

        //read ato file to check what has happened
        readAtoFileBoxDetails();

        //if the number of first atoms is different to the number of components in the atofiletable, then assume unsuccessful
        if (firstAtomList.count() != ui.atoFileTable->rowCount())
        {
            //roll back to previous box.ato file
            QFile::remove(workingDir_+atoFileName_);
            QFile::rename(workingDir_+atoFileName_+".copy", workingDir_+atoFileName_);

            //set number in box for atoFilesToAdd to 0;
            for (int i = 0; i < ui.atoFileTable->rowCount(); i++)
            {
                for (int j = 0; j < atoFilesToAdd.count(); j++)
                {
                    if (atoFilesToAdd.at(j) == ui.atoFileTable->item(i,0)->text())
                    {
                        ui.atoFileTable->item(i,2)->setText("0");
                    }
                }
            }

            messageText_ += "\nAddato unsuccessful\n";
            messagesDialog.refreshMessages();
            ui.messagesLineEdit->setText("Addato unsuccessful");

            QMessageBox msgBox;
            msgBox.setText("Addato unsuccessful - check Settings->EPSR messages for details.");
            msgBox.exec();
            return;
        }

        //otherwise update everything accordingly
        messageText_ += "\nAddato successful - finished writing "+atoFileName_+" file\n";
        messagesDialog.refreshMessages();
        ui.messagesLineEdit->setText("Addato successful - Finished writing box .ato file");

        //roll back to previous box.ato file
        QFile::remove(workingDir_+atoFileName_+".copy");

        //update ui.atoFileTable to include number of mols added for relevant .ato files
        if (container != atoFileName_)
        {
            for (int i = 0; i < ui.atoFileTable->rowCount(); i++)
            {
                if (ui.atoFileTable->item(i,0)->text() == container)
                {
                    ui.atoFileTable->item(i,2)->setText("1");
                }
            }
        }

        for (int i = 0; i < ui.atoFileTable->rowCount(); i++)
        {
            for (int j = 0; j < atoFilesToAdd.count(); j++)
            {
                if (atoFilesToAdd.at(j) == ui.atoFileTable->item(i,0)->text())
                {
                    ui.atoFileTable->item(i,2)->setText(numberOfMolecules.at(j));
                }
            }
        }

        ui.boxAtoLabel->setText(atoFileName_);
        checkBoxCharge();
        ui.randomiseButton->setEnabled(true);
        ui.plotBoxAct->setEnabled(true);
        ui.boxCompositionButton->setEnabled(true);
        ui.updateAtoFileButton->setEnabled(true);
        ui.fmoleButton->setEnabled(true);
        ui.atoEPSRButton->setEnabled(true);
        ui.reloadBoxButton->setEnabled(true);
        ui.removeComponentButton->setEnabled(true);
        ui.dataFileBrowseButton->setEnabled(true);
        ui.removeDataFileButton->setEnabled(true);

        ui.deleteBoxAtoFileAct->setEnabled(true);

        //save .pro file
        save();

        jmolFile_.removePath(workingDir_);
        dir.setPath(workingDir_);
        QStringList jmolFilter;
        jmolFilter << "*.jmol";
        QStringList jmolFiles = dir.entryList(jmolFilter, QDir::Files);
        foreach (QString jmolFile, jmolFiles)
        {
            jmolFile_.removePath(jmolFile);
        }

        BoxCompositionDialog boxCompositionDialog(this);

        boxCompositionDialog.show();
        boxCompositionDialog.raise();
        boxCompositionDialog.activateWindow();

        boxCompositionDialog.exec();
    }
}

void MainWindow::on_loadBoxButton_clicked (bool checked)
{
    //must manually change moltypeXX to the name of the associated .mol file at the bottom of the box.ato file before starting load box
    QMessageBox msgBox;
    msgBox.setText("In the following dialog window, choose the .ato file to be used as the simulation box.\nEnsure it has the components listed at the bottom of the file, instead of moltypeXX.");
    msgBox.exec();

    QString atoFile = QFileDialog::getOpenFileName(this, "Choose EPSR box .ato file", workingDir_, tr(".ato files (*.ato)"));
    if (!atoFile.isEmpty())
    {
        QString atoFilePath = QFileInfo(atoFile).path()+"/";
        atoFilePath = QDir::toNativeSeparators(atoFilePath);
        QFileInfo fi(atoFile);
        atoFileName_ = fi.fileName();

        if (atoFilePath != workingDir_)
        {
            if (QFile::exists(workingDir_+atoFileName_) == true)
            {
                QMessageBox::StandardButton msgBox;
                msgBox  = QMessageBox::question(this, "Warning", "This will overwrite the .ato file already present in the project folder with the same name.\nProceed?", QMessageBox::Ok|QMessageBox::Cancel);
                if (msgBox == QMessageBox::Cancel)
                {
                    return;
                }
                else
                {
                    QFile::copy(atoFile, workingDir_+atoFileName_);
                }
            }
            else
            {
                QFile::copy(atoFile, workingDir_+atoFileName_);
            }
        }

        //run changeato to make sure it is in the correct format
        QDir::setCurrent(workingDir_);
        QString atoBaseFileName = atoFileName_.split(".",QString::SkipEmptyParts).at(0);

        processEPSR_.setProcessChannelMode(QProcess::ForwardedChannels);
    #ifdef _WIN32
        processEPSR_.start(epsrBinDir_+"changeato.exe", QStringList() << workingDir_ << "changeato" << atoBaseFileName);
    #else
        processEPSR_.startDetached(epsrBinDir_+"changeato", QStringList() << workingDir_ << "changeato" << atoBaseFileName);
    #endif
        if (!processEPSR_.waitForStarted()) return;
        processEPSR_.write("e\n");
        processEPSR_.write("\n");
        processEPSR_.write("y\n");
        if (!processEPSR_.waitForFinished(60000)) return;

        //get all the details from the box .ato file
        readAtoFileBoxDetails();

        //check if the component .mol files are in the project folder and moltype isn't at the bottom of the file
        for (int i = 0; i < atoComponentList.count(); i++)
        {
            QFile checkFile(workingDir_+atoComponentList.at(i)+".mol");
            if (!checkFile.exists())
            {
                atoFileName_.clear();
                ui.numberDensityLineEdit->clear();
                ui.boxAtoLabel->clear();
                ui.boxAtoCharge->clear();
                ui.boxAtoMols->clear();
                ui.boxAtoAtoms->clear();
                ui.boxAtoLengthA->clear();
                ui.boxAtoLengthB->clear();
                ui.boxAtoLengthC->clear();
                ui.boxAtoAxisA->clear();
                ui.boxAtoAxisB->clear();
                ui.boxAtoAxisG->clear();
                ui.boxAtoVol->clear();
                ui.temperatureLineEdit->clear();
                ui.vibtempLineEdit->clear();
                ui.angtempLineEdit->clear();
                ui.dihtempLineEdit->clear();
                ui.atoTetherTable->clearContents();
                ui.atoTetherTable->setRowCount(0);
                atoFileName_.clear();
                baseFileName_.clear();
                ui.ecoreLineEdit->clear();
                ui.dcoreLineEdit->clear();

                QMessageBox msgBox;
                msgBox.setText("Could not open find one of the component .mol files.\nCheck all component .mol and .ato files are in the project folder and that the box does not have moltypeXX at the bottom and try again.");
                msgBox.exec();
                return;
            }
        }

        //clear molFileList and atoFileTable
        ui.molFileList->clear();
        ui.atoFileTable->clearContents();

        //update molFilList and atoFileTable
        nMolFiles = atoComponentList.count();
        ui.atoFileTable->setRowCount(nMolFiles);
        for (int i = 0; i < nMolFiles; i++)
        {
            ui.molFileList->addItem(atoComponentList.at(i)+".mol");
            QTableWidgetItem *item = new QTableWidgetItem(atoComponentList.at(i)+".ato");
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            ui.atoFileTable->setItem(i,0, item);
            ui.atoFileTable->setItem(i,2, new QTableWidgetItem(QString::number(nInBox.at(i))));
            ui.molFileList->setCurrentRow(i);
        }

        ui.boxAtoLabel->setText(atoFileName_);
        checkBoxCharge();
        ui.randomiseButton->setEnabled(true);
        ui.plotBoxAct->setEnabled(true);
        ui.boxCompositionButton->setEnabled(true);
        ui.updateAtoFileButton->setEnabled(true);
        ui.fmoleButton->setEnabled(true);
        ui.atoEPSRButton->setEnabled(true);
        ui.reloadBoxButton->setEnabled(true);
        ui.removeComponentButton->setEnabled(true);
        ui.dataFileBrowseButton->setEnabled(true);
        ui.removeDataFileButton->setEnabled(true);
        ui.deleteBoxAtoFileAct->setEnabled(true);

        ui.molFileTabWidget->setEnabled(true);
        ui.viewMolFileButton->setEnabled(true);
        ui.removeMolFileButton->setEnabled(true);
        ui.updateMolFileButton->setEnabled(true);
        ui.mixatoButton->setEnabled(true);
        ui.addatoButton->setEnabled(true);
        ui.loadBoxButton->setEnabled(true);

        messageText_ += "\n"+atoFileName_+" box loaded\n";
        messagesDialog.refreshMessages();
        ui.messagesLineEdit->setText("Box loaded");

        //save .pro file
        save();
    }
}

void MainWindow::on_makelatticeatoButton_clicked(bool checked)
{
    if (ui.atoFileTable->rowCount() == 0)
    {
        return;
    }

    if (ui.atoFileTable->rowCount() >= 2)
    {
        QMessageBox msgBox;
        msgBox.setText("Only one component can be made into a molecular lattice.\nAfter the molecular lattice has been created, use Addato to add additional components.");
        msgBox.exec();
    }

    MakeMolLattDialog makeMolLattDialog(this);

    makeMolLattDialog.setModal(true);
    makeMolLattDialog.show();
    makeMolLattDialog.raise();
    makeMolLattDialog.activateWindow();

    mollattDialog = makeMolLattDialog.exec();

    if (mollattDialog == MakeMolLattDialog::Accepted)
    {
        //get ato file for expanding into a lattice
        QString atoFileName = makeMolLattDialog.getAtoFile();

        //list ato files in folder alphabetically and get the one that is listed in EPSRgui as a component
        QDir::setCurrent(workingDir_);
        QDir dir;
        QStringList atoFilter;
        atoFilter << "*.ato";
        QStringList atoFiles = dir.entryList(atoFilter, QDir::Files, QDir::Name|QDir::IgnoreCase);
        QString atoFileIndex = QString::number(atoFiles.indexOf(atoFileName));
        if (atoFileIndex == "-1")
        {
            QMessageBox msgBox;
            msgBox.setText("The component .ato file could not be found in the working directory.\n");
            msgBox.exec();
            return;
        }

        //get number of cells to multiply along each axis from dialog
        int aCells = makeMolLattDialog.aCells();
        int bCells = makeMolLattDialog.bCells();
        int cCells = makeMolLattDialog.cCells();

        QString projDir = workingDir_;
        projDir = QDir::toNativeSeparators(projDir);
#ifdef _WIN32
        processEPSR_.start(epsrBinDir_+"makelatticeato.exe", QStringList() << projDir << "makelatticeato");
#else
        processEPSR_.start(epsrBinDir_+"makelatticeato", QStringList() << projDir << "makelatticeato");
#endif
        if (!processEPSR_.waitForStarted()) return;

        // press enter to get to ato file listed in the table that will be added to the container
        int newlines = atoFileIndex.toInt();
        for (int nl = 0; nl < newlines; nl++)
        {
            processEPSR_.write("\n");
        }
        processEPSR_.write("y\n");

        processEPSR_.write(qPrintable(QString::number(aCells)+" "+QString::number(bCells)+" "+QString::number(cCells)+"\n"));

        processEPSR_.write(qPrintable(projectName_+"box\n"));

        processEPSR_.write("n\n");

        if (!processEPSR_.waitForFinished(1800000)) return;

        atoFileName_ = projectName_+"box.ato";
        if (readAtoFileBoxDetails() == false)
        {
            atoFileName_.clear();
            QMessageBox msgBox;
            msgBox.setText("Could not read box .ato file.");
            msgBox.exec();
            return;
        }

        //update molFilList and atoFileTable
        ui.molFileList->clear();
        ui.atoFileTable->clearContents();
        ui.atoFileTable->setRowCount(0);
        nMolFiles = atoComponentList.count();
        ui.atoFileTable->setRowCount(nMolFiles);
        for (int i = 0; i < nMolFiles; i++)
        {
            ui.molFileList->addItem(atoComponentList.at(i)+".mol");
            QTableWidgetItem *item = new QTableWidgetItem(atoComponentList.at(i)+".ato");
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            ui.atoFileTable->setItem(i,0, item);
            ui.atoFileTable->setItem(i,2, new QTableWidgetItem(QString::number(nInBox.at(i))));
            ui.molFileList->setCurrentRow(i);
        }

        ui.boxAtoLabel->setText(atoFileName_);
        checkBoxCharge();
        ui.randomiseButton->setEnabled(true);
        ui.plotBoxAct->setEnabled(true);
        ui.boxCompositionButton->setEnabled(true);
        ui.updateAtoFileButton->setEnabled(true);
        ui.fmoleButton->setEnabled(true);
        ui.atoEPSRButton->setEnabled(true);
        ui.reloadBoxButton->setEnabled(true);
        ui.removeComponentButton->setEnabled(true);
        ui.dataFileBrowseButton->setEnabled(true);
        ui.removeDataFileButton->setEnabled(true);
        ui.deleteBoxAtoFileAct->setEnabled(true);

        messageText_ += "\nMolecular lattice made\n";
        messagesDialog.refreshMessages();
        ui.messagesLineEdit->setText("Molecular lattice made");

        //save .pro file
        save();

        jmolFile_.removePath(workingDir_);
        dir.setPath(workingDir_);
        QStringList jmolFilter;
        jmolFilter << "*.jmol";
        QStringList jmolFiles = dir.entryList(jmolFilter, QDir::Files);
        foreach (QString jmolFile, jmolFiles)
        {
            jmolFile_.removePath(jmolFile);
        }
    }
}

bool MainWindow::readAtoFileBoxDetails()
{
    QFile file(workingDir_+atoFileName_);
    if(!file.open(QFile::ReadOnly | QFile::Text))
    {
        QMessageBox msgBox;
        msgBox.setText("Could not open box .ato file.");
        msgBox.exec();
        return false;
    }

    QTextStream stream(&file);
    QString line;
    QStringList dataLine;
    dataLine.clear();
    atoHeaderLines = 0;
    QStringList fullFirstAtomsList;
    fullFirstAtomsList.clear();

    line = stream.readLine();
    dataLine = line.split(" ", QString::SkipEmptyParts);
    QString numberMol = dataLine.at(0);
    if (dataLine.count() == 3)
    {
        double boxLength = dataLine.at(1).toDouble();
        QString boxLengthstr;
        boxLengthstr.sprintf("%.4f", boxLength);
        ui.boxAtoLengthA->setText(boxLengthstr);
        ui.boxAtoLengthB->clear();
        ui.boxAtoLengthC->clear();
        ui.boxAtoAxisA->setText("90.0000"); //polar angles in degrees
        ui.boxAtoAxisB->setText("0.0000");
        ui.boxAtoAxisG->setText("0.0000");
//        cryst angles therefore 90,90,90 deg
        double boxVol = boxLength*boxLength*boxLength;
        QString boxVolstr;
        boxVolstr.sprintf("%.2f", boxVol);
        ui.boxAtoVol->setText(boxVolstr);
        ui.temperatureLineEdit->setText(dataLine.at(2));
        atoHeaderLines = 2;
    }
    else
    {
        ui.temperatureLineEdit->setText(dataLine.at(1));

        line = stream.readLine();
        dataLine = line.split(" ", QString::SkipEmptyParts);
        double boxa = dataLine.at(0).toDouble();
        double boxb = dataLine.at(1).toDouble();
        double boxc = dataLine.at(2).toDouble();
        QString boxastr;
        boxastr.sprintf("%.4f", boxa);
        QString boxbstr;
        boxbstr.sprintf("%.4f", boxb);
        QString boxcstr;
        boxcstr.sprintf("%.4f", boxc);
        ui.boxAtoLengthA->setText(boxastr);
        ui.boxAtoLengthB->setText(boxbstr);
        ui.boxAtoLengthC->setText(boxcstr);       

        line = stream.readLine();
        dataLine = line.split(" ", QString::SkipEmptyParts);
        double boxpb = dataLine.at(0).toDouble();
        double boxtc = dataLine.at(1).toDouble();
        double boxpc = dataLine.at(2).toDouble();
        QString boxpbstr;
        boxpbstr.sprintf("%.4f", boxpb);
        QString boxtcstr;
        boxtcstr.sprintf("%.4f", boxtc);
        QString boxpcstr;
        boxpcstr.sprintf("%.4f", boxpc);
        ui.boxAtoAxisA->setText(boxpbstr);  //polar angles in degrees
        ui.boxAtoAxisB->setText(boxtcstr);
        ui.boxAtoAxisG->setText(boxpcstr);
        //cryst angles therefore:
        // gamma = phib (boxpb)
        // convert all angles to radians
        // bx = cos gammar
        // by = sin gammar
        // phicr = tan-1<2??>(cy,cx)
        // betar = cos-1 cx
        // thetacr = sin-1 (sqrt(cx^2+cy^2))
        // alphar = bos-1 (bc)
        // more maths... need to know what atan2 means in fortran (arc tan in rad but what is the 2??)
        double boxVol = boxa*boxb*sin(boxpb*3.14159265/180)*boxc*cos(boxtc*3.14159265/180);
        QString boxVolstr;
        boxVolstr.sprintf("%.2f", boxVol);
        ui.boxAtoVol->setText(boxVolstr);
        atoHeaderLines = 4;
    }
    line = stream.readLine();
    dataLine = line.split("  ", QString::SkipEmptyParts);
    ui.atoTetherTolLineEdit->setText(dataLine.at(0).trimmed());
    ui.intraTransSSLineEdit->setText(dataLine.at(1));
    ui.grpRotSSLineEdit->setText(dataLine.at(2));
    ui.molRotSSLineEdit->setText(dataLine.at(3));
    ui.molTransSSLineEdit->setText(dataLine.at(4));
    ui.vibtempLineEdit->setText(dataLine.at(5));
    ui.angtempLineEdit->setText(dataLine.at(6));
    ui.dihtempLineEdit->setText(dataLine.at(7));

    QStringList tetherAtoms;
    QStringList tetherList;
    firstAtomList.clear();
    tetherAtoms.clear();
    tetherList.clear();
    int atomctr = 0;
    atoComponentList.clear();
    atoAtomTypes.clear();
    numberAtomTypes.clear();
    QList<int> fullnLinesPerComponentList;
    fullnLinesPerComponentList.clear();
    int lineCtr = 0;
    QRegExp atomLabelrx(" ([A-Z][A-Za-z0-9 ]{2})   ([0-9 ]{1,4})      0");
    QRegExp ecoredcorerx("  ([0-9]{1}[.]{1}[0-9]{1,}[E+-]{2}[0-9]{1,})  ([0-9]{1}[.]{1}[0-9]{1,}[E+-]{2}[0-9]{1,})");

    do {
        line = stream.readLine();
        dataLine = line.split(" ",QString::SkipEmptyParts);
        lineCtr++;
        if (dataLine.count() > 9)
        {
            if (line.contains("T") == true || line.contains("F") == true)
            {
                fullnLinesPerComponentList.append(lineCtr);
                lineCtr = 1;
                if (line.contains("T") == true)
                {
                    tetherList.append("T");     //append to list that this is tethered
                    dataLine = line.split(" ",QString::SkipEmptyParts);
                    QString string = dataLine.at(7);
                    QString tetherAtomStr = string.remove("T");
                    int tetherAtom = tetherAtomStr.toInt();
                    tetherAtomStr = QString::number(tetherAtom);
                    tetherAtoms.append(tetherAtomStr);      //append to list which atom/COM tethering is from
                }
                else
                {
                    tetherList.append("F");     //append to list that this is tethered)
                    tetherAtoms.append(" ");
                }

                line = stream.readLine();   //move to next line to check what the first atom of the molecule is
                dataLine = line.split(" ",QString::SkipEmptyParts);
               // lineCtr++;
                fullFirstAtomsList.append(dataLine.at(0).trimmed());    //get a list of every first atom in the box
                QString tetherMol = dataLine.at(0);
                if (firstAtomList.contains(tetherMol) == true) //check if atom is already listed
                {
                    tetherList.removeLast();    //if it is, remove T/F and 0000X entries in lists
                    tetherAtoms.removeLast();
                }
                else
                {
                    firstAtomList.append(tetherMol);   //if not, add to list
                }
            }
        }
        //get number of each atom Type in box
        if (atomLabelrx.exactMatch(line))
        {
            atomctr++;
            dataLine = line.split("  ", QString::SkipEmptyParts);
            QString data = dataLine.at(0).trimmed();
            if (atoAtomTypes.isEmpty())
            {
                atoAtomTypes.append(data);
                numberAtomTypes.resize(atoAtomTypes.count());
            }
            else
            {
                int test = 0;
                for (int i = 0; i < atoAtomTypes.count(); i++)
                {
                    if (data == atoAtomTypes.at(i))
                    {
                        test = 1;
                    }
                }
                if (test == 0)
                {
                    atoAtomTypes.append(data);
                    numberAtomTypes.resize(atoAtomTypes.count());
                }
            }
            for (int i = 0; i < atoAtomTypes.count(); i++)
            {
                if (data == atoAtomTypes.at(i))
                {
                    numberAtomTypes.operator [](i)++; //add 1 to 'counter' each time find each atom type
                }
            }
        }
    } while (!ecoredcorerx.exactMatch(line));

    dataLine = line.split("  ", QString::SkipEmptyParts);
    ui.ecoreLineEdit->setText(dataLine.at(0));
    ui.dcoreLineEdit->setText(dataLine.at(1));

    line = stream.readLine(); //this is the line of random(?) large numbers
    do {
        line = stream.readLine();
        dataLine = line.split(" ", QString::SkipEmptyParts);
        if (dataLine.count() == 0) break;
        if (dataLine.at(0) == "Atomic") break;
        atoComponentList.append(dataLine.at(1));
    } while (!line.isNull());

    file.close();

    //get number of each component as listed in the box .ato file
    nInBox.clear();
    nInBox.resize(firstAtomList.count());
    for (int i = 0; i < fullFirstAtomsList.count(); i++)
    {
        for (int j = 0; j < firstAtomList.count(); j++)
        {
            if (fullFirstAtomsList.at(i) == firstAtomList.at(j))
            {
                nInBox.operator [](j)++;
            }
        }
    }

    //get numerical position of each component in box
    firstInstance.clear();
    lastInstance.clear();
    firstInstance.append(1);
    lastInstance.append(nInBox.at(0));
    for (int i = 1; i < ui.molFileList->count(); i++) //write atoFileList and then change this to atoFileList******************************************
    {
        firstInstance.append(lastInstance.at(i-1)+1);
        lastInstance.append(firstInstance.at(i)+nInBox.at(i)-1);
    }

    //get number of lines for each component
    nLinesPerComponentList.clear();
    fullnLinesPerComponentList.removeAt(0);
    for (int i = 0; i < ui.molFileList->count(); i++) //write atoFileList and then change this to atoFileList******************************************
    {
        nLinesPerComponentList.append(fullnLinesPerComponentList.at(firstInstance.at(i)));
    }

    QString numberAtom = QString::number(atomctr);
    ui.boxAtoMols->setText(numberMol);
    ui.boxAtoAtoms->setText(numberAtom);

    ui.atoTetherTable->setRowCount(firstAtomList.count());
    for (int i = 0; i < firstAtomList.count(); i++)
    {
        ui.atoTetherTable->setItem(i,0, new QTableWidgetItem(firstAtomList.at(i)));
        ui.atoTetherTable->setItem(i,1, new QTableWidgetItem(tetherList.at(i)));
        ui.atoTetherTable->setItem(i,2, new QTableWidgetItem(tetherAtoms.at(i)));
    }

    //calculate atomic number density and input into box
    double totalNumberAtoms = ui.boxAtoAtoms->text().toDouble();
    double boxVolume = ui.boxAtoVol->text().toDouble();
    double numberDensity = totalNumberAtoms/boxVolume;
    QString numberDensityStr;
    numberDensityStr.sprintf("%.6f", numberDensity);
    ui.numberDensityLineEdit->setText(numberDensityStr);

    //make partials array so can get number of atom pair combinations
    const int NatoAtomTypes = atoAtomTypes.count();
    ij.initialise(NatoAtomTypes,NatoAtomTypes);
    int count = 0;
    for (int i = 0; i < NatoAtomTypes; ++i)
    {
        for (int j = i; j < NatoAtomTypes; ++j)
        {
            ij.ref(i,j) = count;
            ij.ref(j,i) = count;
            ++count;
        }
    }
    nPartials = count;
    return true;
}

void MainWindow::on_boxCompositionButton_clicked(bool checked)
{
    BoxCompositionDialog boxCompositionDialog(this);

    boxCompositionDialog.show();
    boxCompositionDialog.raise();
    boxCompositionDialog.activateWindow();

    boxCompositionDialog.exec();
}

void MainWindow::on_randomiseButton_clicked(bool checked)
{
        QDir::setCurrent(workingDir_);

        QString atoBaseFileName = atoFileName_.split(".",QString::SkipEmptyParts).at(0);

#ifdef _WIN32
        processEPSR_.start(epsrBinDir_+"randomise.exe", QStringList() << workingDir_ << "randomise" << atoBaseFileName);
#else
        processEPSR_.start(epsrBinDir_+"randomise", QStringList() << workingDir_ << "randomise" << atoBaseFileName);
#endif
        if (!processEPSR_.waitForStarted()) return;

        if (!processEPSR_.waitForFinished(1800000)) return;
        messageText_ += "\nfinished randomising box .ato file\n";
        messagesDialog.refreshMessages();
        ui.messagesLineEdit->setText("Finished randomising box");
}

void MainWindow::on_updateAtoFileButton_clicked(bool checked)
{
    if (changeatoFinishedTimerId_ != -1) killTimer(changeatoFinishedTimerId_);
    changeatoFinishedTimerId_ = -1;

    if (ui.temperatureLineEdit->text().isEmpty() || ui.vibtempLineEdit->text().isEmpty() || ui.angtempLineEdit->text().isEmpty() || ui.dihtempLineEdit->text().isEmpty()
            || ui.intraTransSSLineEdit->text().isEmpty() || ui.grpRotSSLineEdit->text().isEmpty() || ui.molRotSSLineEdit->text().isEmpty()
            || ui.molTransSSLineEdit->text().isEmpty() || ui.ecoreLineEdit->text().isEmpty() || ui.dcoreLineEdit->text().isEmpty() || ui.atoTetherTolLineEdit->text().isEmpty())
    {
        QMessageBox msgBox;
        msgBox.setText("One of the parameters defining the simulation box is missing");
        msgBox.exec();
        return;
    }

    QRegExp noNegIntrx("^\\d*$");
    for (int i = 0; i < ui.atoTetherTable->rowCount(); i++)
    {
        if (ui.atoTetherTable->item(i,1)->text().contains("T"))
        {
            if (ui.atoTetherTable->item(i,2)->text().isEmpty() || !noNegIntrx.exactMatch(ui.atoTetherTable->item(i,2)->text()))
            {
                QMessageBox msgBox;
                msgBox.setText("Input the integer label of the atom to be tethered or use 0 for centre of mass");
                msgBox.exec();
                return;
            }
        }
    }

    double atoTemp = ui.temperatureLineEdit->text().toDouble();
    double vibTemp = ui.vibtempLineEdit->text().toDouble();
    double angTemp = ui.angtempLineEdit->text().toDouble();
    double dihTemp = ui.dihtempLineEdit->text().toDouble();
    double intraTransSS = ui.intraTransSSLineEdit->text().toDouble();
    double grpRotSS = ui.grpRotSSLineEdit->text().toDouble();
    double molRotSS = ui.molRotSSLineEdit->text().toDouble();
    double molTransSS = ui.molTransSSLineEdit->text().toDouble();
    double ecore = ui.ecoreLineEdit->text().toDouble();
    double dcore = ui.dcoreLineEdit->text().toDouble();
    double tetherTol = ui.atoTetherTolLineEdit->text().toDouble();
    QString atoTempstr;
    QString vibTempstr;
    QString angTempstr;
    QString dihTempstr;
    QString intraTransSSstr;
    QString grpRotSSstr;
    QString molRotSSstr;
    QString molTransSSstr;
    QString ecorestr;
    QString dcorestr;
    QString tetherTolstr;
    //QStringList tetherMolAtoms;
    QStringList tetherAtoms;
    QStringList tetherList;

    for (int i = 0; i < ui.atoTetherTable->rowCount(); i++)
    {
        //tetherMolAtoms.append(ui.atoTetherTable->item(i,0)->text());
        tetherList.append(ui.atoTetherTable->item(i,1)->text());
        int tetherAtom = ui.atoTetherTable->item(i,2)->text().toInt();
        tetherAtoms.append(QString("%1").arg(tetherAtom, 5, 10, QChar('0')));
    }

    QDir::setCurrent(workingDir_);

    QFile fileRead(workingDir_+atoFileName_);
    QFile fileWrite(workingDir_+"temp.txt");

    if(!fileRead.open(QFile::ReadOnly | QFile::Text))
    {
        QMessageBox msgBox;
        msgBox.setText("Could not open box.ato file");
        msgBox.exec();
        return;
    }
    if(!fileWrite.open(QFile::WriteOnly | QFile::Text))
    {
        QMessageBox msgBox;
        msgBox.setText("Could not open temporary file");
        msgBox.exec();
        return;
    }

    QTextStream streamRead(&fileRead);
    QTextStream streamWrite(&fileWrite);
    QString line;
    QStringList dataLine;
    dataLine.clear();
    QString original;

    QRegExp ecoredcorerx("  ([0-9]{1}[.]{1}[0-9]{1,}[E+-]{2}[0-9]{1,})  ([0-9]{1}[.]{1}[0-9]{1,}[E+-]{2}[0-9]{1,})");

    //1st line of ato file
    line = streamRead.readLine();
    dataLine = line.split(" ", QString::SkipEmptyParts);
    if (dataLine.count() == 0) return;
    //for cubic boxes, there are 3 items on the first line - #mols, box length and temp
    if (dataLine.count() == 3)
    {
        streamWrite << "   " << dataLine.at(0) << "  " << dataLine.at(1) << "  " << atoTempstr.setNum(atoTemp,'E',5) << "\n";
    }
    //for non-cubic boxes there are 2 items on the first line - #mols and temp
    //the next 2 lines are the axes lengths and angles
    else
    {
         streamWrite << "   " << dataLine.at(0) << "  " << atoTempstr.setNum(atoTemp,'E',5) << "\n";
         line = streamRead.readLine();
         streamWrite << line << "\n";
         line = streamRead.readLine();
         streamWrite << line << "\n";
    }

    //vibtemp etc line
    line = streamRead.readLine();
    dataLine = line.split(" ", QString::SkipEmptyParts);
    streamWrite << "   " << tetherTolstr.setNum(tetherTol,'E',5)
           << "  " << intraTransSSstr.setNum(intraTransSS,'E',5)
           << "  " << grpRotSSstr.setNum(grpRotSS,'E',5)
           << "  " << molRotSSstr.setNum(molRotSS,'E',5)
           << "  " << molTransSSstr.setNum(molTransSS,'E',5)
           << "  " << vibTempstr.setNum(vibTemp,'E',5)
           << "  " << angTempstr.setNum(angTemp,'E',5)
           << "  " << dihTempstr.setNum(dihTemp,'E',5)
           << "  " << dataLine.at(8)
           << "  " << dataLine.at(9)<< "\n";

    // rest of file
    while (!streamRead.atEnd())
    {
        line = streamRead.readLine();
        dataLine = line.split(" ",QString::SkipEmptyParts);
        //if a tether line, change tethering in accordance with the atoTetherTable
        if (dataLine.count() > 9)
        {
            QString tetherline = line;
            QString tetherLetter = dataLine.at(7);
            line = streamRead.readLine();
            dataLine = line.split(" ", QString::SkipEmptyParts);
            for (int i = 0; i < firstAtomList.count(); i++)
            {
                if (dataLine.at(0) == firstAtomList.at(i))
                {
                    if (tetherList.at(i) == "T" && tetherLetter == "F")
                    {
//                         tetherline = tetherline.replace(91, 6, "T"+tetherAtoms.at(0));
			tetherline.replace("F     ", "T"+tetherAtoms.at(0));
                    }
                    else
                    if (tetherList.at(i) == "F" && tetherLetter.contains("T") == true)
                    {
//                         tetherline = tetherline.replace(91, 6, "F     ");
			tetherline.replace(tetherLetter, QString("%1").arg("F", -tetherLetter.length()));
                    }
                }
            }
            original.append(tetherline+"\n"+line+"\n");
        }

        else
        if (dataLine.count() == 2)
        {
            if (ecoredcorerx.exactMatch(line))
            {
                line = "  "+ecorestr.setNum(ecore,'E',5)+"  "+dcorestr.setNum(dcore,'E',5);
            }
            original.append(line+"\n");
        }
        else
        {
            original.append(line+"\n");
        }
    }
    fileWrite.resize(0);
    streamWrite << original;
    fileWrite.close();
    fileRead.close();

    //rename temp file as box .ato file to copy over changes and delete temp file
    QFile::remove(workingDir_+atoFileName_);
    QFile::rename(workingDir_+"temp.txt", workingDir_+atoFileName_);

    readAtoFileBoxDetails();

    messageText_ += "\nbox .ato file updated\n";
    messagesDialog.refreshMessages();
    ui.messagesLineEdit->setText("Finished updating box .ato file");
}

void MainWindow::on_fmoleButton_clicked(bool checked)
{
    if (changeatoFinishedTimerId_ != -1) killTimer(changeatoFinishedTimerId_);
    changeatoFinishedTimerId_ = -1;

    int fmoleIter = ui.fmoleLineEdit->text().toInt();

    QDir::setCurrent(workingDir_);

    QString atoBaseFileName = atoFileName_.split(".",QString::SkipEmptyParts).at(0);

    QProcess processFmole;
    processFmole.setProcessChannelMode(QProcess::ForwardedChannels);
#ifdef _WIN32
    processFmole.startDetached(epsrBinDir_+"fmole.exe", QStringList() << workingDir_ << "fmole" << atoBaseFileName << qPrintable(QString::number(fmoleIter)));
#else
    processFmole.startDetached(epsrBinDir_+"fmole", QStringList() << workingDir_ << "fmole" << atoBaseFileName << qPrintable(QString::number(fmoleIter)));
#endif
    messageText_ += "\nfmole is running on box.ato file\n";
    messagesDialog.refreshMessages();
    ui.messagesLineEdit->setText("Running fmole in separate window");

    fmoleFinishedTimerId_ = startTimer(2000);
    QFileInfo fi(workingDir_+atoFileName_);
    atoLastMod_ = fi.lastModified();

    //disable buttons
    ui.fmoleButton->setDisabled(true);
    ui.fmoleLineEdit->setDisabled(true);
    ui.updateAtoFileButton->setDisabled(true);
    ui.mixatoButton->setDisabled(true);
    ui.addatoButton->setDisabled(true);
    ui.loadBoxButton->setDisabled(true);
    ui.randomiseButton->setDisabled(true);
    ui.atoEPSRButton->setDisabled(true);
    ui.updateMolFileButton->setDisabled(true);
    ui.runMenu->setDisabled(true);
}

void MainWindow::on_atoEPSRButton_clicked(bool checked)
{   
    QMessageBox::StandardButton msgBox;
    msgBox  = QMessageBox::question(this, "Warning", "This will run changeato for the simulation box in a terminal window.\nProceed?", QMessageBox::Ok|QMessageBox::Cancel);
    if (msgBox == QMessageBox::Cancel)
    {
        return;
    }
    else
    {
        QDir::setCurrent(workingDir_);

        QString atoBaseFileName = atoFileName_.split(".",QString::SkipEmptyParts).at(0);

        QProcess processEditAto;
        processEditAto.setProcessChannelMode(QProcess::ForwardedChannels);
    #ifdef _WIN32
        processEditAto.startDetached(epsrBinDir_+"changeato.exe", QStringList() << workingDir_ << "changeato" << atoBaseFileName);
    #else
        processEditAto.startDetached(epsrBinDir_+"changeato", QStringList() << workingDir_ << "changeato" << atoBaseFileName);
    #endif
        ui.messagesLineEdit->setText("Changeato opened in separate window");

        changeatoFinishedTimerId_ = startTimer(2000);
        atoLastMod_ = QDateTime::currentDateTime();
    }
}

bool MainWindow::checkBoxCharge()
{
    double boxCharge = 0;
    for (int i = 0; i < ui.atoFileTable->rowCount(); i++)
    {
        QString molChargeCalcdStr = ui.atoFileTable->item(i,1)->text();
        if (molChargeCalcdStr == "n/a")
        {
            ui.boxAtoCharge->setText("not available");
            return false;
        }
        QString molNumberStr = ui.atoFileTable->item(i,2)->text();
        double molChargeCalcd = QString(molChargeCalcdStr).toDouble();
        int molNumber = QString(molNumberStr).toInt();
        boxCharge = boxCharge + (molChargeCalcd*molNumber);
    }
    QString boxChargeStr;
    boxChargeStr.sprintf("%.4f", boxCharge);
    ui.boxAtoCharge->setText(boxChargeStr);
    return false;
}

void MainWindow::on_SSbutton_clicked(bool checked)
{
    if (ui.SSgrpBox->isVisible() == true)
    {
        ui.SSgrpBox->setVisible(false);
    }
    else
    {
        ui.SSgrpBox->setVisible(true);
    }
}

void MainWindow::on_tetherButton_clicked(bool checked)
{
    if (ui.tetherGrpBox->isVisible() == true)
    {
        ui.tetherGrpBox->setVisible(false);
    }
    else
    {
        ui.tetherGrpBox->setVisible(true);
    }
}

void MainWindow::on_reloadBoxButton_clicked(bool checked)
{
    readAtoFileBoxDetails();
}

void MainWindow::on_removeComponentButton_clicked(bool checked)
{
    if (ui.atoFileTable->rowCount() == 0)
    {
        return;
    }

    QMessageBox msgBox;
    msgBox.setText("After removing a component weights files may need to be remade.");
    msgBox.exec();

    readAtoFileBoxDetails();

    RemoveComponentDialog removeComponentDialog(this);

    removeComponentDialog.setModal(true);
    removeComponentDialog.show();
    removeComponentDialog.raise();
    removeComponentDialog.activateWindow();

    componentremoveDialog = removeComponentDialog.exec();

    if (componentremoveDialog == RemoveComponentDialog::Accepted)
    {
        //get component row
        int componentToRemove = removeComponentDialog.returnComponent();  //row is row in atoFileList

        //get range of lines to remove
        QList<int> totlinesPerComponentList;
        totlinesPerComponentList.clear();
        for (int i = 0; i < ui.molFileList->count(); i++) //write atoFileList and then change this to atoFileList******************************************
        {
            totlinesPerComponentList.append(nLinesPerComponentList.at(i)*nInBox.at(i));
        }

        int firstLineToRemove = atoHeaderLines;
        for (int i = 0; i < componentToRemove; i++)
        {
            firstLineToRemove = firstLineToRemove+totlinesPerComponentList.at(i);
        }
        int lastLineToRemove = atoHeaderLines;
        for (int i = 0; i < componentToRemove+1; i++) //write atoFileList and then change this to atoFileList******************************************
        {
            lastLineToRemove = lastLineToRemove+totlinesPerComponentList.at(i);
        }

        //get list of atomTypesToRemove from component.ato file
        QFile fileCompRead(workingDir_+ui.atoFileTable->item(componentToRemove,0)->text());
        if(!fileCompRead.open(QFile::ReadOnly | QFile::Text))
        {
            QMessageBox msgBox;
            msgBox.setText("Could not open component.ato file");
            msgBox.exec();
            return;
        }

        QTextStream streamCompRead(&fileCompRead);
        QString lineComp;
        QRegExp atomlabelrx(" ([A-Z][A-Za-z0-9 ]{2}) ([A-Za-z ]{1,2})   ([0-1]{1})");
        QStringList atomTypesToRemove;
        atomTypesToRemove.clear();

        do {
            lineComp = streamCompRead.readLine();
            if (atomlabelrx.exactMatch(lineComp))
            {
                atomTypesToRemove << atomlabelrx.cap(1).trimmed();
            }

        } while (!lineComp.isNull());
        fileCompRead.close();

        //calc new number of components present
        int nTotComponents = ui.boxAtoMols->text().toInt()-nInBox.at(componentToRemove);

        QFile fileRead(workingDir_+atoFileName_);
        QFile fileWrite(workingDir_+"temp.txt");

        if(!fileRead.open(QFile::ReadOnly | QFile::Text))
        {
            QMessageBox msgBox;
            msgBox.setText("Could not open box.ato file");
            msgBox.exec();
            return;
        }
        if(!fileWrite.open(QFile::WriteOnly | QFile::Text))
        {
            QMessageBox msgBox;
            msgBox.setText("Could not open temporary file");
            msgBox.exec();
            return;
        }

        QTextStream streamRead(&fileRead);
        QTextStream streamWrite(&fileWrite);
        QString line;
        QStringList dataLine;
        dataLine.clear();
        int lineCtr = atoHeaderLines;
        int componentCtr = 0;

        //copy over header
        line = streamRead.readLine();
        line = line.replace(2, 6, " "+QString::number(nTotComponents)); //this isn't the correct formatting*******************************
        streamWrite << line << "\n";
        for (int i = 0; i < atoHeaderLines-1; i++)
        {
            line = streamRead.readLine();
            streamWrite << line << "\n";
        }

        //copy over lines that aren't in the remove list
        QRegExp atomPairrx(" ([A-Z][A-Za-z0-9 ]{2}) ([A-Za-z ]{1,2})   ([0-1]{1})");
        do {
            line = streamRead.readLine();
            lineCtr++;
            if (atomPairrx.exactMatch(line))
            {
                break;
            }
            if (lineCtr <= firstLineToRemove || lineCtr >= lastLineToRemove+1)
            {
                dataLine = line.split(" ", QString::SkipEmptyParts);
                if (dataLine.count() == 10)
                {
                    componentCtr++;
                    line = line.replace(109, 6, " "+QString::number(componentCtr)); //this isn't the correct formatting*******************************
                }
                streamWrite << line << "\n";
            }
        } while (!line.isNull());

        //LJ param section
        for (int i = 0; i < atoAtomTypes.count(); i++)
        {
            dataLine = line.split(" ", QString::SkipEmptyParts);
            int test = 0;
            for (int j = 0; j < atomTypesToRemove.count(); j++)
            {
                if (dataLine.at(0) == atomTypesToRemove.at(j))
                {
                    test = 1;
                }
            }
            if (test == 0)
            {
                streamWrite << line << "\n";
                line = streamRead.readLine();
                streamWrite << line << "\n";
            }
            else
            {
                line = streamRead.readLine();
            }
            line = streamRead.readLine();
        }

        //ecordcore and random line
        streamWrite << line << "\n";
        line = streamRead.readLine();
        streamWrite << line << "\n";

        //atom list
        int componentNum = 1;
        for (int i = 0; i < componentToRemove; i++)
        {
            line = streamRead.readLine();
            line = line.replace(0, 4, "  "+QString::number(componentNum)); //this isn't the correct formatting*******************************
            componentNum++;
            streamWrite << line << "\n";
        }
        line = streamRead.readLine();
        for (int i = componentToRemove+1; i < ui.molFileList->count(); i++)
        {
            line = streamRead.readLine();
            line = line.replace(0, 4, "  "+QString::number(componentNum)); //this isn't the correct formatting*******************************
            componentNum++;
            streamWrite << line << "\n";
        }

        //number density line
        line = streamRead.readLine();
        streamWrite << line << "\n";

        //each component number density ****************this isn't correct but its recalculated when EPSR is run********************************
        for (int i = 0; i < componentToRemove; i++)
        {
            line = streamRead.readLine();
            streamWrite << line << "\n";
        }
        line = streamRead.readLine();
        for (int i = componentToRemove+1; i < ui.molFileList->count(); i++)
        {
            line = streamRead.readLine();
            streamWrite << line << "\n";
        }

        fileRead.close();
        fileWrite.close();

        //rename temp file as box .ato file to copy over changes and delete temp file
        QFile::remove(workingDir_+atoFileName_);
        QFile::rename(workingDir_+"temp.txt", workingDir_+atoFileName_);

        readAtoFileBoxDetails();

        //update mol and ato tables with remaining components
        ui.molFileList->takeItem(componentToRemove);
        setSelectedMolFile();
        ui.atoFileTable->removeRow(componentToRemove);
        nMolFiles = ui.molFileList->count();

        save();

        messageText_ += "\nbox .ato file updated\n";
        messagesDialog.refreshMessages();
        ui.messagesLineEdit->setText("Component removed and box .ato file updated");
    }
}
