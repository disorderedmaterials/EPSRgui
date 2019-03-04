#include <QtGui>
#include <QPushButton>
#include <QDebug>
#include <QProcess>
#include <QString>
#include <QFile>

#include "epsrproject.h"
#include "wtscomponent.h"
#include "datafilesettings.h"
#include "exchangeableatomsdialog.h"

void MainWindow::on_dataFileBrowseButton_clicked(bool checked)
{
    QString newDataFileName;
    if (ui.neutronDataRB->isChecked())
    {
        QString filters(".mint01 files (*.mint01);;All files (*.*)");
        QString defaultFilter(".mint01 files (*.mint01)");
        newDataFileName = QFileDialog::getOpenFileName(this, "Choose dataset", workingDir_, filters, &defaultFilter);
    }
    else
    if (ui.xrayDataRB->isChecked())
    {
        QString filters(".int01 files (*.int01);;All files (*.*)");
        QString defaultFilter(".int01 files (*.int01)");
        newDataFileName = QFileDialog::getOpenFileName(this, "Choose dataset", workingDir_, filters, &defaultFilter);
    }
    if (!newDataFileName.isEmpty())
    {
        QString dataFilePath = QFileInfo(newDataFileName).path()+"/";
        dataFilePath = QDir::toNativeSeparators(dataFilePath);
        QFileInfo dataFileInfo(newDataFileName);
        QString dataFileName;
        dataFileName = dataFileInfo.fileName();

        //check if the selected file is already listed as a dataset in the project
        for (int i = 0; i < ui.dataFileTable->rowCount(); i++)
        {
            if (ui.dataFileTable->item(i,0)->text() == dataFileName)
            {
                QMessageBox msgBox;
                msgBox.setText("This data file is already listed in the project.");
                msgBox.exec();
                return;
            }
        }

        //if necessary, copy to workingDir_ (question if a file of the same name is already present there)
        if (dataFilePath != workingDir_)
        {
            if (QFile::exists(workingDir_+dataFileName) == true)
            {
                QMessageBox::StandardButton msgBox;
                msgBox  = QMessageBox::question(this, "Warning", "This will overwrite the data file already present in the project folder with the same name.\nProceed?", QMessageBox::Ok|QMessageBox::Cancel);
                if (msgBox == QMessageBox::Cancel)
                {
                    return;
                }
                else
                {
                    QFile::copy(newDataFileName, workingDir_+dataFileName);
                }
            }
            else
            {
                QFile::copy(newDataFileName, workingDir_+dataFileName);
            }
        }

        wtsBaseFileName_ = dataFileName.split(".",QString::SkipEmptyParts).at(0);

        ui.dataFileLineEdit->setText(newDataFileName);

        dataFileList.append(dataFileName);

        dataFileName_ = dataFileName;
        if (ui.neutronDataRB->isChecked())
        {
            normalisationList.append("0");
            makeNwtsSetup();
            wtsFileList.append("  ");
            readNwtsSetup();
            refreshDataFileTable();
        }
        else
        if (ui.xrayDataRB->isChecked())
        {
            normalisationList.append("2");
            makeXwtsSetup();
            wtsFileList.append("  ");
            readXwtsSetup();
            refreshDataFileTable();
        }
    }
    ui.exchangeableAtomsButton->setEnabled(true);
    ui.makeWtsButton->setEnabled(true);
    ui.setupEPSRButton->setEnabled(true);

    //save .pro file
    save();

    ui.messagesLineEdit->setText("New data file added");
}

bool MainWindow::makeNwtsSetup()
{
    QDir::setCurrent(workingDir_);
    QString atoBaseFileName = atoFileName_.split(".",QString::SkipEmptyParts).at(0);
    QFile file(workingDir_+wtsBaseFileName_+".NWTS.dat");

    //check if wtsBaseFileName_.NWTS.dat already exists
    if (file.exists() == true)
    {
        QMessageBox::StandardButton msgBox;
        msgBox  = QMessageBox::question(this, "Warning", "A weights setup file for this dataset is already present in the project folder.\n"
                                                         "To make a new weights setup file click Ok, or to use the existing file click Cancel", QMessageBox::Ok|QMessageBox::Cancel);
        if (msgBox == QMessageBox::Ok)
        {
            file.remove();
        }
    }

    //create wtsBaseFileName_.NWTS.dat if it doesn't already exist
    if (file.exists() == false)
    {
#ifdef _WIN32
        processEPSR_.start(epsrBinDir_+"upset.exe", QStringList() << workingDir_ << "upset" << "nwts" << wtsBaseFileName_);
#else
        processEPSR_.start(epsrBinDir_+"upset", QStringList() << workingDir_ << "upset" << "nwts" << wtsBaseFileName_);
#endif
        if (!processEPSR_.waitForStarted()) return false;

        processEPSR_.write("\n");          // move to fnameato line
        processEPSR_.write(qPrintable(atoBaseFileName+"\n"));
        processEPSR_.write("\n");
        processEPSR_.write("e\n");
        processEPSR_.write("\n");

        if (!processEPSR_.waitForFinished()) return false;
        messageText_ += "\nfinished making wts setup file\n";
    }

    //read values in NWTS.dat file and write to table and combobox
    messagesDialog.refreshMessages();
    ui.messagesLineEdit->setText("NWTS setup file created");
    return true;
}

bool MainWindow::makeXwtsSetup()
{
    QDir::setCurrent(workingDir_);
    QString atoBaseFileName = atoFileName_.split(".",QString::SkipEmptyParts).at(0);
    QFile file(workingDir_+wtsBaseFileName_+".XWTS.dat");

    //check if wtsBaseFileName_.XWTS.dat already exists
    if (file.exists() == true)
    {
        QMessageBox::StandardButton msgBox;
        msgBox  = QMessageBox::question(this, "Warning", "A weights setup file for this dataset is already present in the project folder.\n"
                                                         "To make a new weights setup file click Ok, or to use the existing file click Cancel", QMessageBox::Ok|QMessageBox::Cancel);
        if (msgBox == QMessageBox::Ok)
        {
            file.remove();
        }
    }

    //create wtsBaseFileName_.XWTS.dat if it doesn't already exist
    if (file.exists() == false)
    {
#ifdef _WIN32
        processEPSR_.start(epsrBinDir_+"upset.exe", QStringList() << workingDir_ << "upset" << "xwts" << wtsBaseFileName_);
#else
        processEPSR_.start(epsrBinDir_+"upset", QStringList() << workingDir_ << "upset" << "xwts" << wtsBaseFileName_);
#endif
        if (!processEPSR_.waitForStarted()) return false;

        processEPSR_.write("\n");          // move to fnameato line
        processEPSR_.write(qPrintable(atoBaseFileName+"\n"));
        processEPSR_.write("\n");
        processEPSR_.write("e\n");
        processEPSR_.write("\n");

        if (!processEPSR_.waitForFinished()) return false;
        messageText_ += "\nfinished making wts setup file\n";
    }
    messagesDialog.refreshMessages();
    ui.messagesLineEdit->setText("XWTS setup file created");
    return true;
}

bool MainWindow::readNwtsSetup()
{
    QString wtsFileName = workingDir_+wtsBaseFileName_+".NWTS.dat";
    QFile file(wtsFileName);
    if(!file.open(QFile::ReadOnly | QFile::Text))
    {
        QMessageBox msgBox;
        msgBox.setText("Could not open wts setup file");
        msgBox.exec();
        return false;
    }
    QTextStream stream(&file);
    QString line;
    QStringList dataLine;
    dataLine.clear();

    wtscomponents.clear();
    int normdataType = 0;

    do
    {
        line = stream.readLine();
        dataLine = line.split(" ", QString::SkipEmptyParts);
        if (dataLine.count() == 0) continue;
            if (dataLine.at(0) == "normtot")
            {
                normdataType = dataLine.at(1).toInt();
            }
            if (dataLine.at(0) == "component")
            {
                wtscomponents.append(WtsComponent());
            }
            if (dataLine.at(0) == "atom")
            {
                wtscomponents.last().atom=dataLine.at(1);
            }
            if (dataLine.at(0) == "atsymbol")
            {
                wtscomponents.last().atsymbol=dataLine.at(1);
            }
            if (dataLine.at(0) == "iexchange")
            {
                wtscomponents.last().iexchange=dataLine.at(1);
            }
            if (dataLine.at(0) == "abundances")
            {
                  if (dataLine.at(3) != "List")
                  {
                        wtscomponents.last().isotope1=dataLine.at(1);
                        wtscomponents.last().abundance1=dataLine.at(2);
                        wtscomponents.last().isotope2=dataLine.at(3);
                        wtscomponents.last().abundance2=dataLine.at(4);
                  }
                  else
                  {
                      wtscomponents.last().isotope1=dataLine.at(1);
                      wtscomponents.last().abundance1=dataLine.at(2);
                  }
            }
    } while (!stream.atEnd());
    file.close();

    const int N_components = wtscomponents.count();

    //show data in Tables and combobox
    ui.atomWtsTable->setColumnCount(6);
    ui.atomWtsTable->setRowCount(N_components);
    QStringList wtsheader;
    wtsheader << "Atom Type" << "Exchangeable?" << "Isotope" << "Abundance" << "Isotope" << "Abundance";
    ui.atomWtsTable->setHorizontalHeaderLabels(wtsheader);
    ui.atomWtsTable->verticalHeader()->setVisible(false);
    ui.atomWtsTable->horizontalHeader()->setVisible(true);
    for (int i = 0; i < N_components; ++i)
    {
        QTableWidgetItem *item = new QTableWidgetItem(wtscomponents.at(i).atom);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        ui.atomWtsTable->setItem(i,0, item);
        ui.atomWtsTable->setItem(i,1, new QTableWidgetItem(wtscomponents.at(i).iexchange));
        ui.atomWtsTable->setItem(i,2, new QTableWidgetItem(wtscomponents.at(i).isotope1));
        ui.atomWtsTable->setItem(i,3, new QTableWidgetItem(wtscomponents.at(i).abundance1));
        ui.atomWtsTable->setItem(i,4, new QTableWidgetItem(wtscomponents.at(i).isotope2));
        ui.atomWtsTable->setItem(i,5, new QTableWidgetItem(wtscomponents.at(i).abundance2));
    }
    ui.atomWtsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui.atomWtsTable->setSelectionMode(QAbstractItemView::SingleSelection);

    //and on comboBox
    ui.normalisationComboBox->setCurrentIndex(normdataType);

    //update normalisationList
    QString normdataTypeStr = QString::number(normdataType);
    int selectedRow = ui.dataFileTable->currentRow();
    if (selectedRow != -1)
    {
        normalisationList.replace(selectedRow, normdataTypeStr);
    }

    return true;
}

bool MainWindow::readXwtsSetup()
{
    QString wtsFileName = workingDir_+wtsBaseFileName_+".XWTS.dat";
    QFile file(wtsFileName);
    if(!file.open(QFile::ReadOnly | QFile::Text))
    {
        QMessageBox msgBox;
        msgBox.setText("Could not open wts setup file");
        msgBox.exec();
        return false;
    }
    QTextStream stream(&file);
    QString line;
    QStringList dataLine;
    dataLine.clear();

    wtscomponents.clear();
    int normdataType = 2;

    do
    {
        line = stream.readLine();
        dataLine = line.split(" ", QString::SkipEmptyParts);
        if (dataLine.count() != 0)
        {
            if (dataLine.at(0) == "normtot")
            {
                normdataType = dataLine.at(1).toInt();
            }
            if (dataLine.at(0) == "component")
            {
                wtscomponents.append(WtsComponent());
            }
            if (dataLine.at(0) == "atom")
            {
                wtscomponents.last().atom=dataLine.at(1);
            }
            if (dataLine.at(0) == "atsymbol")
            {
                wtscomponents.last().atsymbol=dataLine.at(1);
            }
        }
    } while (!stream.atEnd());
    file.close();

    const int N_components = wtscomponents.count();

    //show data in Tables
    ui.atomWtsTable->setColumnCount(1);
    ui.atomWtsTable->setRowCount(N_components);
    QStringList wtsheader;
    wtsheader << "Atom Label";
    ui.atomWtsTable->setHorizontalHeaderLabels(wtsheader);
    ui.atomWtsTable->verticalHeader()->setVisible(false);
    ui.atomWtsTable->horizontalHeader()->setVisible(true);
    for (int i = 0; i < N_components; ++i)
    {
        QTableWidgetItem *item = new QTableWidgetItem(wtscomponents.at(i).atom);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        ui.atomWtsTable->setItem(i,0, item);
    }
    ui.atomWtsTable->setSelectionMode(QAbstractItemView::SingleSelection);

    //and on comboBox
    ui.normalisationComboBox->setCurrentIndex(normdataType);

    //update normalisationList
    QString normdataTypeStr = QString::number(normdataType);
    int selectedRow = ui.dataFileTable->currentRow();
    if (selectedRow != -1)
    {
        normalisationList.at(selectedRow) == normdataTypeStr;
    }
    return true;
}

void MainWindow::on_makeWtsButton_clicked(bool checked)
{
    if (dataFileList.count() == 0)
    {
        return;
    }

    //make weights files
    if (QFile::exists(workingDir_+wtsBaseFileName_+".NWTS.dat"))
    {
        makeNwts();
    }
    if (QFile::exists(workingDir_+wtsBaseFileName_+".XWTS.dat"))
    {
        makeXwts();
    }

    //save .pro file
    save();
}

void MainWindow::makeNwts()
{
    for (int i = 0; i < ui.atomWtsTable->rowCount(); i++)
    {
        if (ui.atomWtsTable->item(i,1)->text().isEmpty() || ui.atomWtsTable->item(i,2)->text().isEmpty() || ui.atomWtsTable->item(i,3)->text().isEmpty())
        {
            QMessageBox msgBox;
            msgBox.setText("One of the parameters defining the scattering weights is missing");
            msgBox.exec();
            return;
        }
        if (!ui.atomWtsTable->item(i,4)->text().isEmpty() && ui.atomWtsTable->item(i,5)->text().isEmpty())
        {
            QMessageBox msgBox;
            msgBox.setText("One of the parameters defining the isotope concentration is missing");
            msgBox.exec();
            return;
        }
        if (ui.atomWtsTable->item(i,4)->text().isEmpty() && !ui.atomWtsTable->item(i,5)->text().isEmpty())
        {
            QMessageBox msgBox;
            msgBox.setText("One of the parameters defining the isotope concentration is missing");
            msgBox.exec();
            return;
        }
    }

    QDir::setCurrent(workingDir_);

    QFile fileRead(workingDir_+wtsBaseFileName_+".NWTS.dat");
    QFile fileWrite(workingDir_+"tempwts.txt");

    if(!fileRead.open(QFile::ReadOnly | QFile::Text))
    {
        QMessageBox msgBox;
        msgBox.setText("Could not open .NWTS.dat file");
        msgBox.exec();
        return;
    }
    if(!fileWrite.open(QFile::WriteOnly | QFile::Text))
    {
        QMessageBox msgBox;
        msgBox.setText("Could not open temporary wts file");
        msgBox.exec();
        return;
    }

    //read normalisation from combo box and write to table
    int dataNormType = ui.normalisationComboBox->currentIndex();
    QString dataNormTypeStr = QString::number(dataNormType);
    int row = ui.dataFileTable->currentRow();

    ui.dataFileTable->item(row,1)->setText(dataNormTypeStr);

    //make normalisationList consistent with dataFileTable normalisation column
    normalisationList.clear();
    for (int i = 0; i < dataFileList.count(); i++)
    {
        normalisationList.append(ui.dataFileTable->item(i,1)->text());
    }

    QString istr;
    const int N_components = wtscomponents.count();

    wtscomponents.clear();

    for (int i = 0; i < N_components; i++)
    {
        wtscomponents.append(WtsComponent());
        wtscomponents.last().atom=ui.atomWtsTable->item(i,0)->text();
        wtscomponents.last().iexchange=ui.atomWtsTable->item(i,1)->text();
        wtscomponents.last().isotope1=ui.atomWtsTable->item(i,2)->text();
        wtscomponents.last().abundance1=ui.atomWtsTable->item(i,3)->text();
        wtscomponents.last().isotope2=ui.atomWtsTable->item(i,4)->text();
        wtscomponents.last().abundance2=ui.atomWtsTable->item(i,5)->text();
    }

    //check atom types are the same as in boxatofile
    if (atoAtomTypes.count() != N_components)
    {
        QMessageBox msgBox;
        msgBox.setText("Mismatch between number of atom types in simulation box and weights setup file");
        msgBox.exec();
        return;
    }
    for (int i = 0; i < N_components; i++)
    {
        if (atoAtomTypes.at(i) != wtscomponents.at(i).atom)
        {
            QMessageBox msgBox;
            msgBox.setText("Mismatch between atom types in simulation box and weights setup file");
            msgBox.exec();
            return;
        }
    }

    QTextStream streamRead(&fileRead);
    QTextStream streamWrite(&fileWrite);
    QString line;
    QStringList dataLine;
    dataLine.clear();
    QString original;

    for (int N_lines = 0; N_lines < 5; N_lines++)
    {
        line = streamRead.readLine();
        original.append(line+"\n");
    }
    line = streamRead.readLine();
    original.remove(line+"\n");
    original.append("normtot     "+dataNormTypeStr+"               Normalise totals to (0 = nothing, 1 = <b>^2, 2 = <b^2> \n");
    for (int N_lines = 0; N_lines < 3; N_lines++)
    {
        line = streamRead.readLine();
        original.append(line+"\n");
    }
    fileWrite.resize(0);
    streamWrite << original;
    for (int i = 0; i < N_components; i++)
    {
        streamWrite << "\ncomponent   "+istr.number(i+1)+"\n\n";
        streamWrite << "atom        "+wtscomponents.at(i).atom+"               Atom label - set from .ato file\n";
        streamWrite << "atsymbol    "+wtscomponents.at(i).atsymbol+"               Atom symbol - set from .ato file\n";
        streamWrite << "iexchange   "+wtscomponents.at(i).iexchange+"               1 if this atom exchanges with other atoms, 0 otherwise\n";
        streamWrite << "abundances  "+wtscomponents.at(i).isotope1+" "+wtscomponents.at(i).abundance1+" "+wtscomponents.at(i).isotope2+" "+wtscomponents.at(i).abundance2+"               List of mass numbers and abundances. 0 for natural isotope\n";
        streamWrite << "isubs       0               This atom isotope substituted? (1 = yes, 0 = no)\n";
        streamWrite << "abund2      0 1               List of mass numbers and abundances for second sample\n";
    }
    streamWrite << "q\n";

    fileWrite.close();
    fileRead.close();

    fileRead.remove();
    fileWrite.rename(workingDir_+wtsBaseFileName_+".NWTS.dat");

#ifdef _WIN32
    processEPSR_.start(epsrBinDir_+"nwts.exe", QStringList() << workingDir_ << "nwts" << wtsBaseFileName_);
#else
    processEPSR_.start(epsrBinDir_+"nwts", QStringList() << workingDir_ << "nwts" << wtsBaseFileName_);
#endif
    if (!processEPSR_.waitForStarted()) return;
    if (!processEPSR_.waitForFinished()) return;

    //check file was created (need to check if modification actually works too***********************)
    QFile wtsfile(workingDir_+wtsBaseFileName_+".NWTStot.wts");
    if (!wtsfile.exists())
    {
        QMessageBox msgBox;
        msgBox.setText("An error occured while EPSR was making the wts file.\n"
                       "Check that the atom types are as listed in the simulation box and there are no errors in the exchaneable atoms or isotope abundances\n");
        msgBox.exec();
        return;
    }

    wtsFileList.replace(row, wtsBaseFileName_+".NWTStot.wts");
    messageText_ += "\nfinished making wts file\n";
    messagesDialog.refreshMessages();
    ui.messagesLineEdit->setText("finished making wts file");
    refreshDataFileTable();
}

void MainWindow::makeXwts()
{
    QDir::setCurrent(workingDir_);

    QFile fileRead(workingDir_+wtsBaseFileName_+".XWTS.dat");
    QFile fileWrite(workingDir_+"tempxwts.txt");

    if(!fileRead.open(QFile::ReadOnly | QFile::Text))
    {
        QMessageBox msgBox;
        msgBox.setText("Could not open .XWTS.dat file");
        msgBox.exec();
        return;
    }
    if(!fileWrite.open(QFile::WriteOnly | QFile::Text))
    {
        QMessageBox msgBox;
        msgBox.setText("Could not open temporary wts file");
        msgBox.exec();
        return;
    }

    //read normalisation from combo box and write to table
    int dataNormType = ui.normalisationComboBox->currentIndex();
    QString dataNormTypeStr = QString::number(dataNormType);
    int row = ui.dataFileTable->currentRow();

    ui.dataFileTable->item(row,1)->setText(dataNormTypeStr);

    //make normalisationList consistent with dataFileTable normalisation column
    normalisationList.clear();
    for (int i = 0; i < dataFileList.count(); i++)
    {
        normalisationList.append(ui.dataFileTable->item(i,1)->text());
    }

    QString istr;
    const int N_components = wtscomponents.count();

//    wtscomponents.clear();                  IF MAKE ANYTHING EDITABLE IN XWTS THEN NEED TO IMPLEMENT THIS
//
//    for (int i = 0; i < N_components; ++i)
//    {
//    wtscomponents.append(WtsComponent());
//    wtscomponents.last().atom=ui.atomWtsTable->item(i,0)->text();
//     }

    //check atom types are the same as in boxatofile
    if (atoAtomTypes.count() != N_components)
    {
        QMessageBox msgBox;
        msgBox.setText("Mismatch between number of atom types in simulation box and weights setup file");
        msgBox.exec();
        return;
    }
    for (int i = 0; i < N_components; i++)
    {
        if (atoAtomTypes.at(i) != wtscomponents.at(i).atom)
        {
            QMessageBox msgBox;
            msgBox.setText("Mismatch between atom types in simulation box and weights setup file");
            msgBox.exec();
            return;
        }
    }

    QTextStream streamRead(&fileRead);
    QTextStream streamWrite(&fileWrite);
    QString line;
    QStringList dataLine;
    dataLine.clear();
    QString original;

    for (int N_lines = 0; N_lines < 3; N_lines++)
    {
        line = streamRead.readLine();
        original.append(line+"\n");
    }
    line = streamRead.readLine();
    original.remove(line+"\n");
    original.append("normtot     "+dataNormTypeStr+"               Normalise totals to (0 = nothing, 1 = <b>^2, 2 = <b^2> \n");
    for (int N_lines = 0; N_lines < 3; N_lines++)
    {
        line = streamRead.readLine();
        original.append(line+"\n");
    }
    fileWrite.resize(0);
    streamWrite << original;
    for (int i = 0; i < N_components-1; i++)
    {
        streamWrite << "\ncomponent   "+istr.number(i+1)+"\n\n";
        streamWrite << "atom        "+wtscomponents.at(i).atom+"               Atom label - set from .ato file\n";
        streamWrite << "atsymbol    "+wtscomponents.at(i).atsymbol+"               Atom symbol - set from .ato file\n";
        streamWrite << "maffq       0               Modified atomic form factors alpha values (0";
        streamWrite << "maffdelta   0               Modified atomic form factors delta value (0)\n\n";
    }
    streamWrite << "q\n";

    fileWrite.close();
    fileRead.close();

    fileRead.remove();
    fileWrite.rename(workingDir_+wtsBaseFileName_+".XWTS.dat");

#ifdef _WIN32
    processEPSR_.start(epsrBinDir_+"xwts.exe", QStringList() << workingDir_ << "xwts" << wtsBaseFileName_);
#else
    processEPSR_.start(epsrBinDir_+"xwts", QStringList() << workingDir_ << "xwts" << wtsBaseFileName_);
#endif
    if (!processEPSR_.waitForStarted()) return;
    if (!processEPSR_.waitForFinished()) return;

    //check file was created (need to check if modification actually works too***********************)
    QFile wtsfile(workingDir_+wtsBaseFileName_+".XWTS.wts");
    if (!wtsfile.exists())
    {
        QMessageBox msgBox;
        msgBox.setText("Could not make wts file");
        msgBox.exec();
        return;
    }

    wtsFileList.replace(row, wtsBaseFileName_+".XWTS.wts");
    messageText_ += "\nfinished making wts file\n";
    messagesDialog.refreshMessages();
    ui.messagesLineEdit->setText("finished making wts file");
    refreshDataFileTable();
}

void MainWindow::refreshDataFileTable()
{
    int row = ui.dataFileTable->currentRow();
    int nRows = ui.dataFileTable->rowCount();
    ui.dataFileTable->clearContents();
    ui.dataFileTable->verticalHeader()->setVisible(false);
    ui.dataFileTable->horizontalHeader()->setVisible(true);

    if (!dataFileList.isEmpty())
    {
        ui.dataFileTable->setRowCount(dataFileList.count());
        for (int i = 0; i < dataFileList.count(); i++)
        {
            QTableWidgetItem *itemdata = new QTableWidgetItem(dataFileList.at(i));
            itemdata->setFlags(itemdata->flags() & ~Qt::ItemIsEditable);
            ui.dataFileTable->setItem(i,0, itemdata);
            QTableWidgetItem *itemnorm = new QTableWidgetItem(normalisationList.at(i));
            itemnorm->setFlags(itemnorm->flags() & ~Qt::ItemIsEditable);
            ui.dataFileTable->setItem(i,1, itemnorm);
            QTableWidgetItem *itemwts = new QTableWidgetItem(wtsFileList.at(i));
            itemwts->setFlags(itemwts->flags() & ~Qt::ItemIsEditable);
            ui.dataFileTable->setItem(i,2, itemwts);
        }
        ui.dataFileTable->setColumnWidth(0, 300);
        ui.dataFileTable->setColumnWidth(1, 90);
        ui.dataFileTable->setColumnWidth(2, 300);

        ui.dataFileTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        ui.dataFileTable->setSelectionMode(QAbstractItemView::SingleSelection);
        if (dataFileList.count() == nRows)
        {
            ui.dataFileTable->setCurrentCell(row,0);
        }
        else
        {
            ui.dataFileTable->setCurrentCell(dataFileList.count()-1,0);
        }
    }
}

void MainWindow::on_dataFileTable_itemSelectionChanged()
{
    int row = ui.dataFileTable->currentRow();
    if (row >= 0)
    {
        setSelectedDataFile();
    }
}

void MainWindow::setSelectedDataFile()
{
    int row = ui.dataFileTable->currentRow();
    QString dataFileName = dataFileList.at(row);
    QString wtsFileName = wtsFileList.at(row);
    if (wtsFileName.contains(" "))
    {
        wtsFileName = dataFileName;
    }
    QFileInfo fileInfo(wtsFileName);
    QString justFileName = fileInfo.fileName();
    wtsBaseFileName_ = justFileName.split(".",QString::SkipEmptyParts).at(0);
    dataFileName_ = dataFileName;

    if (QFile::exists(workingDir_+wtsBaseFileName_+".NWTS.dat"))
    {
        readNwtsSetup();
        ui.exchangeableAtomsButton->setEnabled(true);
    }
    else
    if (QFile::exists(workingDir_+wtsBaseFileName_+".XWTS.dat"))
    {
        readXwtsSetup();
        ui.exchangeableAtomsButton->setEnabled(false);
    }
}

void MainWindow::on_removeDataFileButton_clicked(bool checked)
{
    if (!epsrInpFileName_.isEmpty())
    {
        QMessageBox::StandardButton msgBox;
        msgBox =QMessageBox::question(this, "Warning", "An EPSR.inp file already exists for this project.\n"
                                                       " Removing this data file will mean the EPSR .inp file will need to be created again.\n"
                                                       " Press Ok to proceed.", QMessageBox::Ok|QMessageBox::Cancel);
        if (msgBox == QMessageBox::Cancel)
        {
            return;
        }
    }
    int row = ui.dataFileTable->currentRow();
    if (dataFileList.count() > 1)
    {
        dataFileList.takeAt(row);
        wtsFileList.takeAt(row);
        normalisationList.takeAt(row);
        setSelectedDataFile();
        refreshDataFileTable();
    }
    else
    {
        dataFileList.clear();
        wtsFileList.clear();
        normalisationList.clear();
        ui.normalisationComboBox->setCurrentIndex(0);
        ui.atomWtsTable->clearContents();
        ui.atomWtsTable->setRowCount(0);
        ui.dataFileTable->removeRow(0);
        ui.exchangeableAtomsButton->setEnabled(false);
        ui.makeWtsButton->setEnabled(false);
        ui.setupEPSRButton->setEnabled(false);
        ui.dataFileLineEdit->clear();
        ui.exchangeableAtomsButton->setEnabled(false);
    }

    //save .pro file
    save();

    ui.messagesLineEdit->setText("Data file removed");
}

void MainWindow::on_exchangeableAtomsButton_clicked(bool checked)
{
    if (wtsFileList.count() < 2) return;

    ExchangeableAtomsDialog exchangeableAtomsDialog(this);

    exchangeableAtomsDialog.setModal(true);
    exchangeableAtomsDialog.show();
    exchangeableAtomsDialog.raise();
    exchangeableAtomsDialog.activateWindow();

    addExchangeableAtoms = exchangeableAtomsDialog.exec();

    if (addExchangeableAtoms == ExchangeableAtomsDialog::Accepted)
    {
        QString wtsSetupFile = exchangeableAtomsDialog.getWtsFile();

        //open and read weights setup file
        QString wtsFileName = workingDir_+wtsSetupFile;
        QFile file(wtsFileName);
        if(!file.open(QFile::ReadOnly | QFile::Text))
        {
            QMessageBox msgBox;
            msgBox.setText("Could not open wts setup file");
            msgBox.exec();
            return;
        }
        QTextStream stream(&file);
        QString line;
        QStringList dataLine;
        dataLine.clear();

        QStringList exchangeableAtoms;
        exchangeableAtoms.clear();

        do
        {
            line = stream.readLine();
            dataLine = line.split(" ", QString::SkipEmptyParts);
            if (dataLine.count() == 0) continue;
            if (dataLine.at(0) == "iexchange")
            {
                exchangeableAtoms.append(dataLine.at(1));
            }
        } while (!stream.atEnd());
        file.close();

        if (wtscomponents.count() != exchangeableAtoms.count())
        {
            QMessageBox msgBox;
            msgBox.setText("There is a mismatch between the weights setup files");
            msgBox.exec();
            return;
        }

        //show data in Table
        for (int i = 0; i < wtscomponents.count(); ++i)
        {
            ui.atomWtsTable->setItem(i,1, new QTableWidgetItem(exchangeableAtoms.at(i)));
        }
        return;
    }
}

