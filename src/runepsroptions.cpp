#include <QtGui>
#include <QPushButton>
#include <QDebug>
#include <QProcess>
#include <QString>
#include <QFile>

#include "epsrproject.h"
#include "setupoutputdialog.h"

void MainWindow::on_setupOutButton_clicked(bool checked)
{
    setupOutput();
}

void MainWindow::getOutputType()
{
   outputSetupFileType_ = ui.setupOutTypeComboBox->currentText();
   outputSetupFileType_ = outputSetupFileType_.toLower();

   plotSetupFileType_ = ui.setupPlotTypeComboBox->currentText();
   plotSetupFileType_ = plotSetupFileType_.toLower();
   showAvailableFiles();
}

void MainWindow::showAvailableFiles()
{
    //show available analysis routines
//    //get currently selected item, if present, then clear list
//    int routineRow = -1;
//    if (ui.outputAvailableList->count() >= 0)
//    {
//        routineRow = ui.outputAvailableList->currentRow();
//    }
    ui.outputAvailableList->clear();

    QDir dir;
    QStringList outputFilter;
    if (outputSetupFileType_== "triangles")
    {
        outputFilter << "*.tri.dat";
    }
    else
    if (outputSetupFileType_== "torangles")
    {
        outputFilter << "*.tor.dat";
    }
    else
    {
        outputFilter << "*."+outputSetupFileType_+".dat";
    }
    QStringList files = dir.entryList(outputFilter, QDir::Files);
    if (!files.isEmpty())
    {
        for (int i = 0; i < files.count(); i++)
        {
            QString fileName = files.at(i).split(".",QString::SkipEmptyParts).at(0);
            ui.outputAvailableList->addItem(fileName);
        }
        ui.outputAvailableList->setCurrentRow(0);
    }

    //show available plot routines
    ui.plotsAvailableList->clear();

    QStringList plotFilter;
    if (plotSetupFileType_ == "plot3djmol")
    {
        plotFilter << "*.CUBE.txt";
    }
    else
    {
        plotFilter << "*."+plotSetupFileType_+".txt";
    }
    QStringList plotFiles = dir.entryList(plotFilter, QDir::Files);
    if (!plotFiles.isEmpty())
    {
        for (int i = 0; i < plotFiles.count(); i++)
        {
            QString plotFileName = plotFiles.at(i).split(".",QString::SkipEmptyParts).at(0);
            ui.plotsAvailableList->addItem(plotFileName);
        }
        ui.plotsAvailableList->setCurrentRow(0);
    }
}

void MainWindow::getOutputsRunning()
{
    QString baseFileName = epsrInpFileName_.split(".",QString::SkipEmptyParts).at(0);
#ifdef _WIN32
    QFile file(workingDir_+"run"+baseFileName+".bat");
#else
    QFile file(workingDir_+"run"+baseFileName+".sh");
#endif
    if (file.exists() == false)
    {
        return;
    }
    if(!file.open(QFile::ReadOnly | QFile::Text))
    {
        QMessageBox msgBox;
        msgBox.setText("Could not open .bat file");
        msgBox.exec();
        return;
    }

    QTextStream stream(&file);
    QString line;
    QStringList dataLine;
    dataLine.clear();

#ifdef _WIN32
    line = stream.readLine();   //readLine for 4 lines to get to run epsr line
    line = stream.readLine();
    line = stream.readLine();
    line = stream.readLine();
#else
    line = stream.readLine();   //readLine for 5 lines to get to run epsr line
    line = stream.readLine();
    line = stream.readLine();
    line = stream.readLine();
    line = stream.readLine();
#endif

    do {
        line = stream.readLine();
#ifdef _WIN32
        if (line.contains("%EPSRrun%killepsr")) break;
#else
        if (line.contains("\"$EPSRrun\"killepsr")) break;
#endif
        else
        if (line.contains("chains")
                || line.contains("clusters")
                || line.contains("coord")
                || line.contains("mapgr")
                || line.contains("partials")
                || line.contains("rings")
                || line.contains("sdf")
                || line.contains("sdfcube")
                || line.contains("sharm")
                || line.contains("torangles")
                || line.contains("triangles")
                || line.contains("voids"))
        {
            dataLine = line.split(" ", QString::SkipEmptyParts);
            ui.runOutEPSRList->addItem(dataLine.at(2)+": "+dataLine.at(3));
        }
        else
        if (line.contains("writexyz"))
        {
            ui.dlputilsOutCheckBox->setChecked(true);
        }
        else
        {
             ui.additionalCommandsTextEdit->appendPlainText(line);
        }
    }while (!line.isNull());
    file.close();
}

void MainWindow::setupOutput()
{
    if (ui.setupOutNameNew->text().isEmpty() && ui.outputAvailableList->count() == 0)
    {
        QMessageBox msgBox;
        msgBox.setText("Type a name for the new file");
        msgBox.exec();
        return;
    }

    if (!ui.setupOutNameNew->text().isEmpty())
    {
        outputFileName_ = ui.setupOutNameNew->text();
    }
    else
    {
        outputFileName_ = ui.outputAvailableList->currentItem()->text();
    }

    ui.setupOutNameNew->clear();

    if (outputSetupFileType_== "triangles")
    {
        outputFileExt_ = ".TRI.dat";
    }
    else
    if (outputSetupFileType_== "torangles")
    {
        outputFileExt_ = ".TOR.dat";
    }
    else
    {
        outputFileExt_ = "."+outputSetupFileType_.toUpper()+".dat";
    }

    outputType_ = 1;

    QFileInfo fi(workingDir_+outputFileName_+outputFileExt_);
    if (!fi.exists())
    {
        QDir::setCurrent(workingDir_);

        processEPSR_.setProcessChannelMode(QProcess::ForwardedChannels);
#ifdef _WIN32
        processEPSR_.start(epsrBinDir_+"upset.exe", QStringList() << workingDir_ << "upset" << outputSetupFileType_ << outputFileName_);
#else
        processEPSR_.start(epsrBinDir_+"upset", QStringList() << workingDir_ << "upset" << outputSetupFileType_ << outputFileName_);
#endif
        if (!processEPSR_.waitForStarted()) return;

        processEPSR_.write(qPrintable("fnameato "+atoFileName_+"\n"));
        processEPSR_.write("e\n");
        processEPSR_.write("\n");
        if (!processEPSR_.waitForFinished(60000)) return;

        messageText_ += "\nfinished making EPSR output file "+outputFileName_+outputFileExt_+"\n";
        messagesDialog.refreshMessages();
    }

    SetupOutputDialog setupOutputDialog(this);

    setupOutputDialog.show();
    setupOutputDialog.raise();
    setupOutputDialog.activateWindow();
    setupOutputDialog.exec();

    showAvailableFiles();
    ui.messagesLineEdit->setText("EPSR output file edited");
}

void MainWindow::on_addOutputButton_clicked(bool checked)
{
    if (ui.outputAvailableList->count() == 0) return;
    QString fileToAdd = ui.outputAvailableList->currentItem()->text();
    ui.runOutEPSRList->addItem(outputSetupFileType_+": "+fileToAdd);
    addOutputsToScript();
}

void MainWindow::on_removeOutputButton_clicked(bool checked)
{
    if (ui.runOutEPSRList->count() == 0) return;
    int rowToRemove = ui.runOutEPSRList->currentRow();
    ui.runOutEPSRList->takeItem(rowToRemove);
    addOutputsToScript();
}

void MainWindow::on_applyOutputsButton_clicked(bool checked)
{
    addOutputsToScript();
}

void MainWindow::on_applyCommandButton_clicked(bool checked)
{
    addOutputsToScript();
}

void MainWindow::addOutputsToScript()
{
    //make list of file types and names to be included
    QStringList outputFileTypes;
    QStringList outputFileNames;
    outputFileNames.clear();

    if (ui.runOutEPSRList->count() != 0)
    {
        for (int i = 0; i < ui.runOutEPSRList->count(); i++)
        {
            QString fileName = ui.runOutEPSRList->item(i)->text().split(": ", QString::SkipEmptyParts).at(1);
            outputFileNames.append(fileName);
            QString fileType = ui.runOutEPSRList->item(i)->text().split(": ", QString::SkipEmptyParts).at(0);
            outputFileTypes.append(fileType.toLower());
        }
    }

    //open .bat file
    QString baseFileName = epsrInpFileName_.split(".",QString::SkipEmptyParts).at(0);

#ifdef _WIN32
    QFile batFile(workingDir_+"run"+baseFileName+".bat");
#else
    QFile batFile(workingDir_+"run"+baseFileName+".sh");
#endif
    if (batFile.exists() == false)
    {
        QMessageBox msgBox;
        msgBox.setText("The script file doesn't exist yet - run EPSR first");
        msgBox.exec();
        return;
    }
    if (!batFile.open(QFile::ReadWrite | QFile::Text))
    {
        QMessageBox msgBox;
        msgBox.setText("Could not open script file.");
        msgBox.exec();
        return;
    }

    QTextStream stream(&batFile);
    QString lineToAdd;
    QString line;
    QString original;
    QStringList dataLine;
    dataLine.clear();
    original.clear();

    //get 'header' without anything else
#ifdef _WIN32
    for (int i = 0; i < 4; i++) //remove all entries in script file
#else
    for (int i = 0; i < 5; i++) //remove all entries in script file
#endif
    {
        line = stream.readLine();
        original.append(line+"\n");
    }

    //add output routines to be included
    if (!outputFileNames.isEmpty())
    {
        for (int i = 0; i < outputFileNames.count(); i++)
        {
            QString outputFileName = outputFileNames.at(i);
            QString outputFileType = outputFileTypes.at(i);
#ifdef _WIN32
            lineToAdd = "%EPSRbin%"+outputFileType+".exe %EPSRrun% "+outputFileType+" "+outputFileName+"\n";
#else
            lineToAdd = "  \"$EPSRbin\"'"+outputFileType+"' \"$EPSRrun\" "+outputFileType+" "+outputFileName+"\n";
#endif
            original.append(lineToAdd);
        }
    }

    //add writexyz if ticked
    if (ui.dlputilsOutCheckBox->isChecked() == true)
    {
        //get box side length and divide by 2 to get value for bat file **Alan sugggested add 10% to this to be certain to get any molecule that is right on the very edge of the box
        double boxLengthA = ui.boxAtoLengthA->text().toDouble();
        double boxLengthB = ui.boxAtoLengthB->text().toDouble();
        double boxLengthC = ui.boxAtoLengthC->text().toDouble();
        double halfboxLengthA = boxLengthA/2;
        double halfboxLengthB;
        double halfboxLengthC;
        if (ui.boxAtoLengthB->text().isEmpty())
        {
            halfboxLengthB = halfboxLengthA;
        }
        else
        {
            halfboxLengthB = boxLengthB/2;
        }
        if (ui.boxAtoLengthC->text().isEmpty())
        {
            halfboxLengthC = halfboxLengthA;
        }
        else
        {
            halfboxLengthC = boxLengthC/2;
        }

        QString halfboxLengthAStr = QString::number(halfboxLengthA);
        QString halfboxLengthBStr = QString::number(halfboxLengthB);
        QString halfboxLengthCStr = QString::number(halfboxLengthC);
        QString atoBaseFileName = atoFileName_.split(".",QString::SkipEmptyParts).at(0);
#ifdef _WIN32
        lineToAdd = "%EPSRbin%writexyz.exe %EPSRrun% writexyz "+atoBaseFileName+" y 0 "+halfboxLengthAStr+" "+halfboxLengthBStr+" -"+halfboxLengthCStr+" "+halfboxLengthCStr+" 0 0 0 0\n";
#else
        lineToAdd = "  \"$EPSRbin\"'writexyz' \"$EPSRrun\" writexyz "+atoBaseFileName+" y 0 "+halfboxLengthAStr+" "+halfboxLengthBStr+" -"+halfboxLengthCStr+" "+halfboxLengthCStr+" 0 0 0 0\n";
#endif
        original.append(lineToAdd);
    }

    //add additional commands if not empty
    if (!ui.additionalCommandsTextEdit->toPlainText().isEmpty())
    {
        original.append(ui.additionalCommandsTextEdit->toPlainText()+"\n");
    }

    //add 'footer'
#ifdef _WIN32
    original.append("if not exist %EPSRrun%killepsr ( goto loop ) else del %EPSRrun%killepsr\n");
#else
    original.append("  if ([ -e \"$EPSRrun\"killepsr ])\n");
    original.append("  then break\n");
    original.append("  fi\n");
    original.append("done\n");
    original.append("rm -r \"$EPSRrun\"killepsr\n");
#endif

    //write to file
    batFile.resize(0);
    stream.flush();
    stream << original;
    batFile.close();

    ui.messagesLineEdit->setText("script file updated");
}

void MainWindow::on_setupPlotButton_clicked(bool checked)
{
    setupPlot();
}

void MainWindow::setupPlot()
{
    if (ui.setupPlotNameNew->text().isEmpty() && ui.plotsAvailableList->count() == 0)
    {
        QMessageBox msgBox;
        msgBox.setText("Type a name for the new file");
        msgBox.exec();
        return;
    }

    if (!ui.setupPlotNameNew->text().isEmpty())
    {
        plotFileName_ = ui.setupPlotNameNew->text();
    }
    else
    {
        plotFileName_ = ui.plotsAvailableList->currentItem()->text();
    }

    ui.setupPlotNameNew->clear();

    if (plotSetupFileType_== "plot3djmol")
    {
        plotFileExt_ = ".CUBE.txt";
    }
    else
    {
        plotFileExt_ = "."+plotSetupFileType_+".txt";
    }

    QFileInfo fi(workingDir_+plotFileName_+plotFileExt_);
    if (!fi.exists())
    {

        QString coeffFileName = QFileDialog::getOpenFileName(this, "Choose coefficients file", workingDir_, tr(".SHARM.h01 files (*.h01)"));
        if (coeffFileName.isEmpty())
        {
            return;
        }
        QFileInfo fi(coeffFileName);
        coeffFileName_ = fi.fileName();

        QDir::setCurrent(workingDir_);

        processEPSR_.setProcessChannelMode(QProcess::ForwardedChannels);
#ifdef _WIN32
        processEPSR_.start(epsrBinDir_+"upset.exe", QStringList() << workingDir_ << "upset" << plotSetupFileType_ << plotFileName_);
#else
        processEPSR_.start(epsrBinDir_+"upset", QStringList() << workingDir_ << "upset" << plotSetupFileType_ << plotFileName_);
#endif
        if (!processEPSR_.waitForStarted()) return;

        processEPSR_.write(qPrintable("shcoeffs "+coeffFileName_+"\n"));
        processEPSR_.write("e\n");
        processEPSR_.write("\n");
        if (!processEPSR_.waitForFinished(60000)) return;

        messageText_ += "\nfinished making EPSR output file "+plotFileName_+plotFileExt_+"\n";
        messagesDialog.refreshMessages();
    }

    outputType_ = 2;

    SetupOutputDialog setupOutputDialog(this);

    setupOutputDialog.show();
    setupOutputDialog.raise();
    setupOutputDialog.activateWindow();
    setupOutputDialog.exec();

    showAvailableFiles();
    ui.messagesLineEdit->setText("EPSR output file edited");

//    outputTimerId_ = startTimer(2000);
}
