#include <QPushButton>
#include <QTableWidget>
#include <QTextStream>
#include <QString>

#include "setupoutputdialog.h"
#include "epsrproject.h"

SetupOutputDialog::SetupOutputDialog(MainWindow *parent) : QDialog(parent)
{
    ui.setupUi(this);

    ui.calcGroupBox->setEnabled(false);
    ui.outputSettingsTable->clear();
    ui.outputSettingsTable->setRowCount(0);
    ui.calcSettingsTable->clear();
    ui.calcSettingsTable->setRowCount(0);

    mainWindow_ = parent;

    workingDir_ = mainWindow_->workingDir();
    epsrBinDir_ = mainWindow_->epsrBinDir();
    epsrBinDir_ = QDir::toNativeSeparators(epsrBinDir_);

    outputType_ = mainWindow_->showOutputType();
    if (outputType_ == 1)
    {
        atoFileName_ = mainWindow_->atoFileName();
        ui.atoFileLabel->setText(atoFileName_);
        outputFileName_ = mainWindow_->outputFileName();
        outputSetupFileType_ = mainWindow_->outputSetupFileType();
        outputSetupFileType_ = outputSetupFileType_.toUpper();
        outputFileExt_ = mainWindow_->outputFileExt();
        this->setWindowTitle("Setup Output Routine: "+outputSetupFileType_+" "+outputFileName_);
        ui.atoORcoeffLabel->setText("Simulation box name:");
        ui.browseButton->setDisabled(true);
    }
    else
    if (outputType_ == 2)
    {
        coeffFileName_ = mainWindow_->coeffFileName();
        outputFileName_ = mainWindow_->plotFileName();
        outputSetupFileType_ = mainWindow_->plotSetupFileType();
        outputFileExt_ = mainWindow_->plotFileExt();
        this->setWindowTitle("Setup Plot Routine: "+outputSetupFileType_+" "+outputFileName_);
        ui.atoORcoeffLabel->setText("Coefficients file name:");
        ui.browseButton->setEnabled(true);
    }

    settingEntries_ = 0;
    linesPerEntry_ = 0;
    entry_ = 0;
    outputKeywords.clear();
    outputValues.clear();
    outputDescriptions.clear();

    connect(ui.saveButton, SIGNAL(clicked()), this, SLOT(writeOutputFile()));
    connect(ui.cancelButton, SIGNAL(clicked()), this, SLOT(reject()));

    readOutputSetupFile();
    updateTables();
}

void SetupOutputDialog::readOutputSetupFile()
{
    QFile file(workingDir_+outputFileName_+outputFileExt_);
    if (!file.open(QFile::ReadOnly | QFile::Text))
    {
        QMessageBox msgBox;
        msgBox.setText("Could not open setup output file "+workingDir_+outputFileName_+outputFileExt_);
        msgBox.exec();
        return;
    }

    QTextStream stream(&file);
    QString line;
    QStringList dataLine;
    dataLine.clear();

    outputKeywords.clear();
    outputValues.clear();
    outputDescriptions.clear();
    QRegExp rx("^.{12}");

    //read in first group of settings
    if (outputSetupFileType_ == "plot2d" || outputSetupFileType_ == "plot3d")
    {
        if (outputSetupFileType_ == "plot2d")
        {
            line = stream.readLine();
            dataLine = line.split("               ", QString::SkipEmptyParts);
            outputValues.append(dataLine.at(0));
            outputDescriptions.append(dataLine.at(1));
        }
        line = stream.readLine();   //get name of coeff file
        coeffFileName_ = line;
        ui.atoFileLabel->setText(coeffFileName_);

        do {
            line = stream.readLine();
            dataLine = line.split("               ", QString::SkipEmptyParts);
            if (dataLine.count() < 2)
            {
                outputValues.append(line);
                outputDescriptions.append(" ");
            }
            else
            {
                outputValues.append(dataLine.at(0));
                outputDescriptions.append(dataLine.at(1));
            }
        } while (!stream.atEnd());

        //remove the ".SHARM.h01" line
        outputValues.removeLast();
        outputDescriptions.removeLast();
    }
    else
    if (outputSetupFileType_ == "splot2d")
    {
        line = stream.readLine(); //title line
        line = stream.readLine(); //shcoeffs line
        do {
            line = stream.readLine();
            QString start = line;
            start.truncate(12);
            outputKeywords.append(start);
            dataLine = line.split("               ", QString::SkipEmptyParts);
            if (dataLine.count() == 0)
            {
                outputValues.append(" ");
                outputDescriptions.append(" ");
            }
            else
            {
                QString firstPart = dataLine.at(0);
                outputValues.append(firstPart.split(rx, QString::SkipEmptyParts).at(0));
                outputDescriptions.append(dataLine.at(1));
            }
        } while (line != "q");

        //remove the "q" line
        outputKeywords.removeLast();
        outputValues.removeLast();
        outputDescriptions.removeLast();
    }
    else
    {
        line = stream.readLine(); //title line
        line = stream.readLine(); //fnameato/shcoeffs line
        if (outputType_ == 2)
        {
            dataLine = line.split("               ", QString::SkipEmptyParts);
            QString firstPart = dataLine.at(0);
            coeffFileName_ = firstPart.split(rx, QString::SkipEmptyParts).at(0);
            ui.atoFileLabel->setText(coeffFileName_);
        }

        do {
            line = stream.readLine();
            QString start = line;
            start.truncate(12);
            outputKeywords.append(start);
            dataLine = line.split("               ", QString::SkipEmptyParts);
            if (dataLine.count() < 2)
            {
                outputValues.append(" ");
                outputDescriptions.append(" ");
            }
            else
            {
                QString firstPart = dataLine.at(0);
                outputValues.append(firstPart.split(rx, QString::SkipEmptyParts).at(0));
                outputDescriptions.append(dataLine.at(1));
            }
        } while (line != "q");

        //remove the "q" line
        outputKeywords.removeLast();
        outputValues.removeLast();
        outputDescriptions.removeLast();
    }
    file.close();
}

void SetupOutputDialog::updateTables()
{

    if (outputSetupFileType_ == "plot2d" || outputSetupFileType_ =="plot3d")
    {
        settingEntries_ = outputValues.count();

        ui.outputSettingsTable->clear();
        ui.outputSettingsTable->setColumnWidth(0,180);
        ui.outputSettingsTable->setColumnWidth(1,600);
        ui.outputSettingsTable->setColumnCount(2);
        ui.outputSettingsTable->setRowCount(settingEntries_);
        ui.outputSettingsTable->horizontalHeader()->setVisible(true);
        QStringList header;
        header << "Value" << "Description";
        ui.outputSettingsTable->setHorizontalHeaderLabels(header);
        ui.outputSettingsTable->verticalHeader()->setVisible(false);
        for (int i = 0; i < settingEntries_; i++)
        {
            ui.outputSettingsTable->setItem(i,0, new QTableWidgetItem(outputValues.at(i)));
            QTableWidgetItem *itemdescrip = new QTableWidgetItem(outputDescriptions.at(i));
            itemdescrip->setFlags(itemdescrip->flags() & ~Qt::ItemIsEditable);
            ui.outputSettingsTable->setItem(i,1, itemdescrip);
        }
        ui.outputSettingsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        ui.outputSettingsTable->setSelectionMode(QAbstractItemView::SingleSelection);
        ui.outputSettingsTable->setCurrentCell(settingEntries_-1,0);
    }
    else
    if (outputSetupFileType_ == "splot2d")
    {
        settingEntries_ = 0; //this is the number of entries that aren't part of calculations, and doesn't include the title line, the fnameato line or the nXXX line if present
        for (int i = 0; i < outputKeywords.count(); i++)
        {
            if (outputKeywords.at(i).contains("ncolour"))
            {
                break;
            }
            settingEntries_++;
        }
        int ncolourCalcs = outputValues.at(settingEntries_).toInt();

        ui.outputSettingsTable->clear();
        ui.outputSettingsTable->setColumnWidth(1,180);
        ui.outputSettingsTable->setColumnWidth(2,600);
        ui.outputSettingsTable->setColumnCount(3);
        ui.outputSettingsTable->setRowCount(outputKeywords.count()-(5*ncolourCalcs)-1);
        ui.outputSettingsTable->horizontalHeader()->setVisible(true);
        QStringList header;
        header << "Keyword" << "Value" << "Description";
        ui.outputSettingsTable->setHorizontalHeaderLabels(header);
        ui.outputSettingsTable->verticalHeader()->setVisible(false);
        for (int i = 0; i < settingEntries_; i++)
        {
            QTableWidgetItem *itemkeyword = new QTableWidgetItem(outputKeywords.at(i));
            itemkeyword->setFlags(itemkeyword->flags() & ~Qt::ItemIsEditable);
            ui.outputSettingsTable->setItem(i,0, itemkeyword);
            ui.outputSettingsTable->setItem(i,1, new QTableWidgetItem(outputValues.at(i)));
            QTableWidgetItem *itemdescrip = new QTableWidgetItem(outputDescriptions.at(i));
            itemdescrip->setFlags(itemdescrip->flags() & ~Qt::ItemIsEditable);
            ui.outputSettingsTable->setItem(i,2, itemdescrip);
        }

        for (int i = settingEntries_+(5*ncolourCalcs)+1; i < outputValues.count(); i++)
        {
            QTableWidgetItem *itemkeyword = new QTableWidgetItem(outputKeywords.at(i));
            itemkeyword->setFlags(itemkeyword->flags() & ~Qt::ItemIsEditable);
            ui.outputSettingsTable->setItem(i-(5*ncolourCalcs)-1,0, itemkeyword);
            ui.outputSettingsTable->setItem(i-(5*ncolourCalcs)-1,1, new QTableWidgetItem(outputValues.at(i)));
            QTableWidgetItem *itemdescrip = new QTableWidgetItem(outputDescriptions.at(i));
            itemdescrip->setFlags(itemdescrip->flags() & ~Qt::ItemIsEditable);
            ui.outputSettingsTable->setItem(i-(5*ncolourCalcs)-1,2, itemdescrip);
        }

        ui.outputSettingsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        ui.outputSettingsTable->setSelectionMode(QAbstractItemView::SingleSelection);
        ui.outputSettingsTable->setCurrentCell(outputKeywords.count()-(5*ncolourCalcs)-2,0);
    }
    else
    {
        settingEntries_ = 0; //this is the number of entries that aren't part of calculations, and doesn't include the title line, the fnameato line or the nXXX line if present
        for (int i = 0; i < outputKeywords.count(); i++)
        {
            if (outputKeywords.at(i).contains("ndist") || outputKeywords.at(i).contains("nsphere") || outputKeywords.contains("nviews"))
            {
                break;
            }
            settingEntries_++;
        }

        ui.outputSettingsTable->clear();
        ui.outputSettingsTable->setColumnWidth(1,180);
        ui.outputSettingsTable->setColumnWidth(2,600);
        ui.outputSettingsTable->setColumnCount(3);
        ui.outputSettingsTable->setRowCount(settingEntries_);
        ui.outputSettingsTable->horizontalHeader()->setVisible(true);
        QStringList header;
        header << "Keyword" << "Value" << "Description";
        ui.outputSettingsTable->setHorizontalHeaderLabels(header);
        ui.outputSettingsTable->verticalHeader()->setVisible(false);
        for (int i = 0; i < settingEntries_; i++)
        {
            QTableWidgetItem *itemkeyword = new QTableWidgetItem(outputKeywords.at(i));
            itemkeyword->setFlags(itemkeyword->flags() & ~Qt::ItemIsEditable);
            ui.outputSettingsTable->setItem(i,0, itemkeyword);
            ui.outputSettingsTable->setItem(i,1, new QTableWidgetItem(outputValues.at(i)));
            QTableWidgetItem *itemdescrip = new QTableWidgetItem(outputDescriptions.at(i));
            itemdescrip->setFlags(itemdescrip->flags() & ~Qt::ItemIsEditable);
            ui.outputSettingsTable->setItem(i,2, itemdescrip);
        }
        ui.outputSettingsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        ui.outputSettingsTable->setSelectionMode(QAbstractItemView::SingleSelection);
        ui.outputSettingsTable->setCurrentCell(settingEntries_-1,0);
    }

    if (outputSetupFileType_ != "PARTIALS" && outputKeywords.count() > settingEntries_) //partials has ndist but actually only 1 distribution is allowed per file.
    {
        //find the number of subheaders
        int subEntries = outputValues.at(settingEntries_).toInt();

        //find number of lines in each subsection
        linesPerEntry_ = 0;
        for (int i = settingEntries_+4; i < outputKeywords.count(); i++)
        {
            if (outputKeywords.at(i).isEmpty() || outputKeywords.at(i).contains("q") || outputKeywords.at(i).contains("ncontour"))
            {
                break;
            }
            linesPerEntry_++;
        }

        ui.calcGroupBox->setEnabled(true);

        for (int i = 0; i < subEntries; i++)
        {
            ui.calcsList->addItem(QString::number(i+1)); //change this to be an atom or something more useful?
        }

        ui.calcsList->setCurrentRow(subEntries-1);
        entry_ = ui.calcsList->currentRow();
    }
}

void SetupOutputDialog::on_calcsList_itemSelectionChanged()
{
    //first copy the values from the table to the stringlists
    if (ui.calcSettingsTable->rowCount() != 0)
    {
        int line = settingEntries_+4+((linesPerEntry_+3)*entry_);
        for (int i = 0; i < linesPerEntry_; i++)
        {
            outputValues.replace(line, ui.calcSettingsTable->item(i,1)->text());
            line++;
        }
    }

    entry_ = ui.calcsList->currentRow();
    if (entry_ > -1)
    {
        ui.calcSettingsTable->clear();
        ui.calcSettingsTable->setColumnWidth(1,180);
        ui.calcSettingsTable->setColumnWidth(2,300);
        ui.calcSettingsTable->setRowCount(linesPerEntry_);
        ui.calcSettingsTable->horizontalHeader()->setVisible(true);
        QStringList header;
        header << "Keyword" << "Value" << "Description";
        ui.calcSettingsTable->setHorizontalHeaderLabels(header);
        ui.calcSettingsTable->verticalHeader()->setVisible(false);

        int line = settingEntries_+4+((linesPerEntry_+3)*entry_);
        for (int i = 0; i < linesPerEntry_; i++)
        {
            QTableWidgetItem *itemkeyword = new QTableWidgetItem(outputKeywords.at(line));
            itemkeyword->setFlags(itemkeyword->flags() & ~Qt::ItemIsEditable);
            ui.calcSettingsTable->setItem(i,0, itemkeyword);
            ui.calcSettingsTable->setItem(i,1, new QTableWidgetItem(outputValues.at(line)));
            QTableWidgetItem *itemdescrip = new QTableWidgetItem(outputDescriptions.at(line));
            itemdescrip->setFlags(itemdescrip->flags() & ~Qt::ItemIsEditable);
            ui.calcSettingsTable->setItem(i,2, itemdescrip);
            line++;
        }

        ui.calcSettingsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        ui.calcSettingsTable->setSelectionMode(QAbstractItemView::SingleSelection);
        ui.calcSettingsTable->setCurrentCell(0,0);
    }
}

void SetupOutputDialog::on_saveButton_clicked(bool checked)
{

}

void SetupOutputDialog::writeOutputFile()
{
    //first copy the values from the table to the stringlists
    if (ui.calcSettingsTable->rowCount() != 0)
    {
        int line = settingEntries_+4+((linesPerEntry_+3)*ui.calcsList->currentRow());
        for (int i = 0; i < linesPerEntry_; i++)
        {
            outputValues.replace(line, ui.calcSettingsTable->item(i,1)->text());
            line++;
        }
    }

    QDir::setCurrent(workingDir_);

    QFile fileWrite(workingDir_+"temp.txt");

    if(!fileWrite.open(QFile::WriteOnly | QFile::Text))
    {
        QMessageBox msgBox;
        msgBox.setText("Could not open temporary file");
        msgBox.exec();
        return;
    }

    QTextStream streamWrite(&fileWrite);
    QString line;
    QString original;

    if (outputType_ == 1)
    {
        streamWrite << outputFileName_ << "." << outputSetupFileType_.toUpper() << "               Title of this file\n"
                    << "fnameato    " << atoFileName_ << "               Name of .ato file\n";

        for (int i = 0; i < settingEntries_; i++)
        {
            line = ui.outputSettingsTable->item(i,0)->text()+ui.outputSettingsTable->item(i,1)->text()+"               "+ui.outputSettingsTable->item(i,2)->text()+"\n";
            original.append(line);
        }
    }
    else
    if (outputType_ == 2)
    {
        if (outputSetupFileType_ == "plot2d")
        {
            streamWrite << ui.outputSettingsTable->item(0,0)->text() << "               " << ui.outputSettingsTable->item(0,1)->text() << "\n"
                        << coeffFileName_ << "\n";

            for (int i = 1; i < settingEntries_; i++)
            {
                line = ui.outputSettingsTable->item(i,0)->text()+"               "+ui.outputSettingsTable->item(i,1)->text()+"\n";
                original.append(line);
            }
        }
        else
        if (outputSetupFileType_ == "plot3d")
        {
            streamWrite << coeffFileName_ << "\n";

            for (int i = 0; i < settingEntries_; i++)
            {
                line = ui.outputSettingsTable->item(i,0)->text()+"               "+ui.outputSettingsTable->item(i,1)->text()+"\n";
                original.append(line);
            }
        }
        else
        {
            QString fileExt = outputFileExt_.split(".", QString::SkipEmptyParts).at(0);
            streamWrite << outputFileName_ << "." << fileExt << "               Title of this file\n"
                        << "shcoeffs    " << coeffFileName_ << "               Name of file containing spherical harmonic coefficients\n";

            for (int i = 0; i < settingEntries_; i++)
            {
                line = ui.outputSettingsTable->item(i,0)->text()+ui.outputSettingsTable->item(i,1)->text()+"               "+ui.outputSettingsTable->item(i,2)->text()+"\n";
                original.append(line);
            }
        }
    }

    if (ui.calcsList->count() > 0)
    {
        original.append(outputKeywords.at(settingEntries_)+QString::number(ui.calcsList->count())+"               "+outputDescriptions.at(settingEntries_)+"\n");
        int entryLine = settingEntries_+1;
        for (int j = 0; j < ui.calcsList->count(); j++)
        {
            original.append("\n");
            entryLine++;
            original.append(outputKeywords.at(settingEntries_+2).split(" ", QString::SkipEmptyParts).at(0)+"   "+QString::number(j+1)+"\n");
            entryLine++;
            original.append("\n");
            entryLine++;
            for (int i = 0; i < linesPerEntry_; i++)
            {
                line = outputKeywords.at(entryLine)+outputValues.at(entryLine)+"               "+outputDescriptions.at(entryLine)+"\n";
                original.append(line);
                entryLine++;
            }
        }
    }

    if (outputSetupFileType_ == "splot2d")
    {
        int ncolourCalcs = ui.calcsList->count();
        for (int i = settingEntries_+(5*ncolourCalcs); i < outputKeywords.count()-1; i++)
        {
            line = ui.outputSettingsTable->item(i-(5*ncolourCalcs),0)->text()+ui.outputSettingsTable->item(i-(5*ncolourCalcs),1)->text()+"               "+ui.outputSettingsTable->item(i-(5*ncolourCalcs),2)->text()+"\n";
            original.append(line);
        }
    }

    if (outputSetupFileType_ == "plot2d" || outputSetupFileType_ == "plot3d")
    {
        original.append(".SHARM.h01\n");
    }
    else
    if (outputSetupFileType_ == "PARTIALS")
    {
        original.append("ndist       1               Number of site-site distributions\n");
        original.append("q\n");
    }
    else
    {
        original.append("q\n");
    }
    fileWrite.resize(0);
    streamWrite << original;
    fileWrite.close();

    //rename temp file as box .ato file to copy over changes and delete temp file
    QFile::remove(workingDir_+outputFileName_+outputFileExt_);
    QFile::rename(workingDir_+"temp.txt", workingDir_+outputFileName_+outputFileExt_);

    accept();
}

void SetupOutputDialog::on_cancelButton_clicked(bool checked)
{

}

void SetupOutputDialog::on_addCalcButton_clicked(bool checked)
{
    int calcToAdd = ui.calcsList->count()+1;
    ui.calcsList->addItem(QString::number(calcToAdd));

    if (outputSetupFileType_ == "splot2d")
    {
        int line = settingEntries_+1;
        for (int i = 0; i < linesPerEntry_+3; i++)
        {
            outputKeywords.insert(line+((linesPerEntry_+3)*(calcToAdd-1)), outputKeywords.at(line));
            outputValues.insert(line+((linesPerEntry_+3)*(calcToAdd-1)), outputValues.at(line));
            outputDescriptions.insert(line+((linesPerEntry_+3)*(calcToAdd-1)), outputDescriptions.at(line));
            line++;
        }
    }
    else
    {
        int line = settingEntries_+1;
        for (int i = 0; i < linesPerEntry_+3; i++)
        {
            outputKeywords.append(outputKeywords.at(line));
            outputValues.append(outputValues.at(line));
            outputDescriptions.append(outputDescriptions.at(line));
            line++;
        }
    }

    ui.calcsList->setCurrentRow(calcToAdd-1);
}

void SetupOutputDialog::on_removeCalcButton_clicked(bool checked)
{
    if (ui.calcsList->count() < 2) return;  //don't allow first calc to be removed as this contains keywords and descriptions

    int calcToTake = ui.calcsList->currentRow();
    ui.calcsList->takeItem(calcToTake);

    int line = settingEntries_+1+((linesPerEntry_+3)*calcToTake);
    for (int i = 0; i < linesPerEntry_+3; i++)
    {
        outputKeywords.removeAt(line);
        outputValues.removeAt(line);
        outputDescriptions.removeAt(line);
    }
}

void SetupOutputDialog::on_deleteOutputButton_clicked(bool checked)
{
    QFile::remove(workingDir_+outputFileName_+outputFileExt_);
    this->close();
}

void SetupOutputDialog::on_browseButton_clicked(bool checked)
{
    QString coeffFileName = QFileDialog::getOpenFileName(this, "Choose coefficients file", workingDir_, tr(".SHARM.h01 files (*.h01)"));
    if (!coeffFileName.isEmpty())
    {
        coeffFileName_ = coeffFileName;
        ui.atoFileLabel->setText(coeffFileName_);
    }
}
