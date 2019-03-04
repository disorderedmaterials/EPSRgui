#include "epsrproject.h"
#include "ui_epsrproject.h"
#include "plotdialog.h"
#include "createnewdialog.h"
#include "moloptionsdialog.h"
#include "settingsdialog.h"
#include "makeatomdialog.h"
#include "makelatticedialog.h"
#include "boxcompositiondialog.h"
#include "addatodialog.h"
#include "messagesdialog.h"
#include "importdialog.h"
#include "plotboxdialog.h"
#include "setupoutputdialog.h"
#include "notesdialog.h"
#include "exchangeableatomsdialog.h"

#include <QtGui>
#include <QMainWindow>
#include <QFileDialog>
#include <QMessageBox>
#include <QAction>
#include <QMenuBar>
#include <QMenu>
#include <QToolBar>
#include <QStatusBar>
#include <QApplication>
#include <QHeaderView>
#include <QWidget>
#include <QContextMenuEvent>
#include <QProcess>
#include <QSettings>

MainWindow::MainWindow(QMainWindow *parent) : QMainWindow(parent), messagesDialog(this)
{
    ui.setupUi(this);

#ifdef Q_OS_MAC
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QStringList envlist = env.toStringList();

    for(int i=0; i < envlist.size(); i++)
    {
        QString entry = envlist[i];

        if(entry.startsWith("PATH="))
        {
            int index = entry.indexOf("=");

            if(index != -1)
            {
                QString value = entry.right(entry.length() - (index+1));
                value += ":/usr/texbin:/usr/local/bin";

                setenv("PATH", value.toLatin1().constData(), true);
            }

            break;
        }
    }
#endif

    exeDir_ = QApplication::applicationDirPath(); //QDir::currentPath() doesn't work on OSX

    createActions();

    ui.tabWidget->setEnabled(false);
    ui.plot1Button->setEnabled(false);
    ui.plot2Button->setEnabled(false);

    ui.fileMenu->setFocus();
    ui.messagesLineEdit->setReadOnly(true);

    //set accepted characters in box tab
    QRegExp numberDensityrx("^\\d*\\.?\\d*$");
    ui.numberDensityLineEdit->setValidator((new QRegExpValidator(numberDensityrx, this)));
    QRegExp noNegIntrx("^\\d*$");
    QRegExp decOrErx("[-+]?[0-9]*[.]?[0-9]+([eE][-+]?[0-9]+)?");
    ui.fmoleLineEdit->setValidator((new QRegExpValidator(noNegIntrx, this)));
    ui.atoTetherTolLineEdit->setValidator((new QRegExpValidator(decOrErx, this)));
    ui.temperatureLineEdit->setValidator((new QRegExpValidator(decOrErx, this)));
    ui.vibtempLineEdit->setValidator((new QRegExpValidator(decOrErx, this)));
    ui.angtempLineEdit->setValidator((new QRegExpValidator(decOrErx, this)));
    ui.dihtempLineEdit->setValidator((new QRegExpValidator(decOrErx, this)));
    ui.intraTransSSLineEdit->setValidator((new QRegExpValidator(decOrErx, this)));
    ui.grpRotSSLineEdit->setValidator((new QRegExpValidator(decOrErx, this)));
    ui.molRotSSLineEdit->setValidator((new QRegExpValidator(decOrErx, this)));
    ui.molTransSSLineEdit->setValidator((new QRegExpValidator(decOrErx, this)));
    ui.ecoreLineEdit->setValidator((new QRegExpValidator(decOrErx, this)));
    ui.dcoreLineEdit->setValidator((new QRegExpValidator(decOrErx, this)));

    //hide ss and tether Grp Boxes
    ui.SSgrpBox->setVisible(false);
    ui.tetherGrpBox->setVisible(false);

    ui.normalisationComboBox->setCurrentIndex(0);
    ui.plotComboBox1->setCurrentIndex(0);
    ui.plotComboBox2->setCurrentIndex(2);
    ui.setupOutTypeComboBox->setCurrentIndex(0);

    QStringList atoheader;
    atoheader << "Component" << "Charge" << "# in box";
    ui.atoFileTable->setHorizontalHeaderLabels(atoheader);
    ui.atoFileTable->verticalHeader()->setVisible(false);
    ui.atoFileTable->horizontalHeader()->setVisible(true);

    molOptionsDialog = 0;
    makeAtomDialog = 0;
    makeLatticeDialog = 0;

    readSettings();

    connect(ui.plot1, SIGNAL(mouseMove(QMouseEvent*)), this, SLOT(showPointToolTip1(QMouseEvent*)));
    connect(ui.plot2, SIGNAL(mouseMove(QMouseEvent*)), this, SLOT(showPointToolTip2(QMouseEvent*)));
    connect(ui.plot1, SIGNAL(mouseWheel(QWheelEvent*)), this, SLOT(plotZoom1(QWheelEvent*)));
    connect(ui.plot2, SIGNAL(mouseWheel(QWheelEvent*)), this, SLOT(plotZoom2(QWheelEvent*)));

    connect(ui.setupOutTypeComboBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(getOutputType()));
    connect(ui.setupPlotTypeComboBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(getOutputType()));
    connect(&processEPSR_, SIGNAL(readyReadStandardOutput()), this, SLOT(outputfromEPSRprocessReady()));
    connect(&epsrFinished_, SIGNAL(fileChanged(const QString &)), this, SLOT(enableButtons()));
    connect(&jmolFile_, SIGNAL(directoryChanged(const QString &)), this, SLOT(makeMolFile()));
    connect(&jmolFile_, SIGNAL(fileChanged(const QString &)), this, SLOT(makeMolFile()));

    // Timer IDs
    molChangeatoFinishedTimerId_ = -1;
    outputTimerId_ = -1;
    molChangeatoFinishedTimerId_ = -1;
}

void MainWindow::createActions()
{
    ui.newAct->setStatusTip(tr("Create a new EPSR project"));
    connect(ui.newAct, SIGNAL(triggered()), this, SLOT(createNew()));

    ui.openAct->setStatusTip(tr("Open an existing EPSR project"));
    connect(ui.openAct, SIGNAL(triggered()), this, SLOT(open()));

    ui.saveAsAct->setStatusTip(tr("Save the current EPSR project as a different name"));
    connect(ui.saveAsAct, SIGNAL(triggered()), this, SLOT(saveAs()));

    ui.importAct->setStatusTip(tr("Import an existing EPSR simulation"));
    connect(ui.importAct, SIGNAL(triggered()), this, SLOT(import()));

    ui.exitAct->setStatusTip(tr("Exit the application"));
    connect(ui.exitAct, SIGNAL(triggered()), this, SLOT(close()));

    ui.plotAct->setStatusTip(tr("Plot EPSR outputs"));
    connect(ui.plotAct, SIGNAL(triggered()), this, SLOT(plot()));

    ui.plotBoxAct->setStatusTip(tr("Plot EPSR simluation box"));
    connect(ui.plotBoxAct, SIGNAL(triggered()), this, SLOT(plotBox()));

    ui.plotEPSRshellAct->setStatusTip(tr("Plot EPSR using EPSRshell"));
    connect(ui.plotEPSRshellAct, SIGNAL(triggered()), this, SLOT(plotEPSRshell()));

    ui.plot3djmolAct->setStatusTip(tr("Plot SHARM or SDF outputs using Jmol"));
    connect(ui.plot3djmolAct, SIGNAL(triggered()), this, SLOT(plotJmol()));

    ui.plot2dAct->setStatusTip(tr("Plot SHARM or SDF outputs as a 2D surface in PGPLOT"));
    connect(ui.plot2dAct, SIGNAL(triggered()), this, SLOT(plot2d()));

    ui.plot3dAct->setStatusTip(tr("Plot SHARM or SDF outputs as a 3D surface in PGPLOT"));
    connect(ui.plot3dAct, SIGNAL(triggered()), this, SLOT(plot3d()));

    ui.splot2dAct->setStatusTip(tr("Plot SHARM or SDF outputs as a 2D surface in gnuplot"));
    connect(ui.splot2dAct, SIGNAL(triggered()), this, SLOT(splot2d()));

    ui.settingsAct->setStatusTip(tr("Change EPSRgui settings"));
    connect(ui.settingsAct, SIGNAL(triggered()), this, SLOT(settings()));

    ui.checkAct->setStatusTip(tr("Run EPSR once"));
    connect(ui.checkAct, SIGNAL(triggered()), this, SLOT(runEPSRonce()));

    ui.runAct->setStatusTip(tr("Run EPSR"));
    connect(ui.runAct, SIGNAL(triggered()), this, SLOT(runEPSR()));

    ui.stopAct->setStatusTip(tr("Stop EPSR"));
    connect(ui.stopAct, SIGNAL(triggered()), this, SLOT(stopEPSR()));

    ui.deleteEPSRinpFileAct->setStatusTip(tr("Delete the existing EPSR .inp file"));
    connect(ui.deleteEPSRinpFileAct, SIGNAL(triggered()), this, SLOT(deleteEPSRinpFile()));

    ui.deleteBoxAtoFileAct->setStatusTip(tr("Delete the existing box .ato file"));
    connect(ui.deleteBoxAtoFileAct, SIGNAL(triggered()), this, SLOT(deleteBoxAtoFile()));

    ui.showMessagesAct->setStatusTip(tr("Show messages from EPSR"));
    connect(ui.showMessagesAct, SIGNAL(triggered()), this, SLOT(showMessages()));

    ui.notesAct->setStatusTip(tr("Write or view project notes"));
    connect(ui.notesAct, SIGNAL(triggered()), this, SLOT(showNotes()));

    ui.epsrManualAct->setStatusTip(tr("Open EPSR manual"));
    connect(ui.epsrManualAct, SIGNAL(triggered()), this, SLOT(openEPSRmanual()));

    ui.epsrguiManualAct->setStatusTip(tr("Open EPSRgui manual"));
    connect(ui.epsrguiManualAct, SIGNAL(triggered()), this, SLOT(openEPSRguiManual()));
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!projectName_.isEmpty())
    {
        if (ui.epsrRunningSign->isEnabled() == true)
        {
            QMessageBox::StandardButton msgBox;
            msgBox  = QMessageBox::question(this, "Warning", "EPSR is currently running. \nTo close EPSRgui and stop EPSR at the end of the current iteration, click OK.",
                                            QMessageBox::Ok|QMessageBox::Cancel);
            if (msgBox == QMessageBox::Cancel)
            {
                event->ignore();
                return;
            }
            else
            {
                QFile file(workingDir_+"killepsr");
                if(!file.open(QFile::WriteOnly))
                {
                    QMessageBox msgBox;
                    msgBox.setText("Could not stop EPSR script");
                    msgBox.exec();
                    return;
                }
                file.close();
            }
        }

        save();

        //delete plotting .bat/.sh files
        QDir::setCurrent(workingDir_);
        QDir dir;
        QStringList batFilter;
    #ifdef _WIN32
        batFilter << "plot*.bat";
    #else
        batFilter << "plot*.sh";
    #endif
        QStringList batFiles = dir.entryList(batFilter, QDir::Files);
        if (!batFiles.isEmpty())
        {
            for (int i = 0; i < batFiles.count(); i++)
            {
                QFile file(batFiles.at(i));
                file.remove();
            }
        }
    }

    event->accept();
}

void MainWindow::readSettings()
{
    QSettings settings;

    epsrDir_ = settings.value("EPSRdir").toString();
    if (!epsrDir_.isEmpty())
    {
        epsrDir_ = QDir::toNativeSeparators(epsrDir_+"/");
    }
    epsrBinDir_ = settings.value("EPSRbindir").toString();
    if (!epsrDir_.isEmpty())
    {
        epsrBinDir_ = QDir::toNativeSeparators(epsrBinDir_+"/");
    }
    visualiserExe_ = settings.value("visualiser").toString();

    epsrDir_ = QDir::toNativeSeparators(epsrDir_);
    epsrBinDir_ = QDir::toNativeSeparators(epsrBinDir_);
    visualiserExe_ = QDir::toNativeSeparators(visualiserExe_);
}

void MainWindow::createNew()
{
    if (epsrBinDir_.isEmpty())
    {
        QMessageBox msgBox;
        msgBox.setText("The EPSR binaries directory needs to be defined.");
        msgBox.exec();
        return;
    }

    CreateNewDialog createNewDialog(this);

    createNewDialog.setModal(true);
    createNewDialog.show();
    createNewDialog.raise();
    createNewDialog.activateWindow();

    newDialog = createNewDialog.exec();

    if (newDialog == CreateNewDialog::Accepted)
    {
        reset();

        //set file directories
        projectName_ = createNewDialog.getEPSRname();
        epsrDir_ = createNewDialog.getPrefDir()+"/";
        epsrDir_ = QDir::toNativeSeparators(epsrDir_);
        workingDir_ = (epsrDir_+projectName_+"/");
        workingDir_ = QDir::toNativeSeparators(workingDir_);
        if (!QDir(workingDir_).exists())
        {
            QDir().mkdir(workingDir_);
        }

        messageText_ += "\n***************************************************************************\n";
        messageText_ += "Current EPSR project name is "+projectName_+"\n";
        messageText_ += "Current working directory is "+workingDir_+"\n";
        messageText_ += "Current EPSR binaries directory is "+epsrBinDir_+"\n";

        QDir::setCurrent(workingDir_);

        //make .EPSR.pro file
        QString epsrProFile = workingDir_+"/"+projectName_+".EPSR.pro";
        QFile file(epsrProFile);
        file.open(QFile::ReadWrite | QFile::Text);
        file.close();

        //copy important EPSR files into working directory
        QString epsrStartupDir = epsrBinDir_.split("bin", QString::SkipEmptyParts).at(0);
        epsrStartupDir = epsrStartupDir+"startup/";
        epsrStartupDir = QDir::toNativeSeparators(epsrStartupDir);
        QFile::copy(epsrStartupDir+"f0_WaasKirf.dat", workingDir_+"f0_WaasKirf.dat");
        QFile::copy(epsrStartupDir+"plot_defaults.txt", workingDir_+"plot_defaults.txt");
        QFile::copy(epsrStartupDir+"vanderWaalsRadii.txt", workingDir_+"vanderWaalsRadii.txt");
        QFile::copy(epsrStartupDir+"gnubonds.txt", workingDir_+"gnubonds.txt");
        QFile::copy(epsrStartupDir+"gnuatoms.txt", workingDir_+"gnuatoms.txt");
#ifdef _WIN32
        QFile::copy(epsrStartupDir+"epsr.bat", workingDir_+"epsr.bat");
        QFile::copy(epsrStartupDir+"system_commands_windows.txt", workingDir_+"system_commands.txt");
#else
        QFile::copy(epsrStartupDir+"epsr", workingDir_+"epsr");
        QFile::copy(epsrStartupDir+"system_commands_linux.txt", workingDir_+"system_commands.txt");
#endif

        //activate tabs
        ui.tabWidget->setEnabled(true);

        //activate menu options
        ui.saveAsAct->setEnabled(true);
        ui.epsrManualAct->setEnabled(true);
        ui.notesAct->setEnabled(true);

        //Activate available outputs list
        getOutputType();

        //change window title to contain projectName
        this->setWindowTitle("EPSRgui: "+projectName_);
        messagesDialog.refreshMessages();
        ui.messagesLineEdit->setText("New project "+projectName_+" created");

        //close any dialog windows open *******************************************************************************************************TO DO

    }
}

void MainWindow::reset()
{
    //clear tables and key names
    projectName_.clear();
    workingDir_.clear();

    ui.molFileList->clear();
    ui.molChargeLabel->clear();
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
    molFileName_.clear();

    ui.atoFileTable->clearContents();
    ui.atoFileTable->setRowCount(0);
    ui.numberDensityLineEdit->clear();
    ui.boxAtoLabel->clear();
    ui.boxAtoCharge->clear();
    ui.boxAtoAtoms->clear();
    ui.boxAtoMols->clear();
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
    ui.ecoreLineEdit->clear();
    ui.dcoreLineEdit->clear();
    ui.atoTetherTable->clearContents();
    ui.atoTetherTable->setRowCount(0);
    atoFileName_.clear();
    baseFileName_.clear();

    ui.dataFileTable->clearContents();
    ui.dataFileTable->setRowCount(0);
    ui.atomWtsTable->clearContents();
    ui.atomWtsTable->setRowCount(0);
    dataFileName_.clear();
    wtsBaseFileName_.clear();
    dataFileExt_.clear();
    dataFileList.clear();
    wtsFileList.clear();
    normalisationList.clear();

    ui.epsrInpFileName->clear();
    ui.autoUpdateCheckBox->setChecked(false);
    ui.inpSettingsTable->clearContents();
    ui.inpSettingsTable->setRowCount(0);
    ui.dataFileSettingsTable->clearContents();
    ui.dataFileSettingsTable->setRowCount(0);
    ui.pcofSettingsTable->clearContents();
    ui.pcofSettingsTable->setRowCount(0);
    ui.minDistanceTable->clearContents();
    ui.minDistanceTable->setRowCount(0);
    epsrInpFileName_.clear();

    ui.runOutEPSRList->clear();
    ui.dlputilsOutCheckBox->setChecked(false);

    //enable/disable buttons
    ui.tabWidget->setCurrentIndex(0);
    ui.molFileTabWidget->setCurrentIndex(0);
    ui.molFileTabWidget->setEnabled(false);
    ui.createMolFileButton->setEnabled(true);
    ui.molFileLoadButton->setEnabled(true);
    ui.createAtomButton->setEnabled(true);
    ui.createLatticeButton->setEnabled(true);
    ui.viewMolFileButton->setEnabled(false);
    ui.removeMolFileButton->setEnabled(false);
    ui.updateMolFileButton->setEnabled(false);
    ui.makeMolExtButton->setEnabled(true);
    ui.addLJRowAboveButton->setEnabled(true);
    ui.addLJRowBelowButton->setEnabled(true);
    ui.deleteLJRowButton->setEnabled(true);
    ui.mixatoButton->setEnabled(false);
    ui.addatoButton->setEnabled(false);
    ui.loadBoxButton->setEnabled(true);
    ui.randomiseButton->setEnabled(false);
    ui.boxCompositionButton->setEnabled(false);
    ui.updateAtoFileButton->setEnabled(false);
    ui.fmoleButton->setEnabled(false);
    ui.atoEPSRButton->setEnabled(false);
    ui.reloadBoxButton->setEnabled(false);
    ui.removeComponentButton->setEnabled(false);
    ui.dataFileBrowseButton->setEnabled(false);
    ui.removeDataFileButton->setEnabled(false);
    ui.exchangeableAtomsButton->setEnabled(false);
    ui.makeWtsButton->setEnabled(false);
    ui.setupEPSRButton->setEnabled(false);
    ui.ESPRtabWidget->setCurrentIndex(0);
    ui.updateInpPcofFilesButton->setEnabled(false);
    ui.reloadEPSRinpButton->setEnabled(false);
    ui.setupOutTypeComboBox->setCurrentIndex(0);
    ui.setupOutButton->setEnabled(false);
    ui.setupPlotTypeComboBox->setCurrentIndex(0);
    ui.applyOutputsButton->setEnabled(false);
    ui.applyCommandButton->setEnabled(false);
    ui.setupPlotButton->setEnabled(false);
    ui.dlputilsOutCheckBox->setEnabled(false);
    ui.plot1Button->setEnabled(false);
    ui.plot2Button->setEnabled(false);

    //clear plots
    ui.plot1->clearGraphs();
    ui.plot1->clearItems();
    ui.plot1->clearPlottables();
    ui.plot1->replot();
    ui.plot2->clearGraphs();
    ui.plot2->clearItems();
    ui.plot2->clearPlottables();
    ui.plot2->replot();

    //disable menu actions
    ui.saveAsAct->setEnabled(false);
    ui.deleteBoxAtoFileAct->setEnabled(false);
    ui.deleteEPSRinpFileAct->setEnabled(false);
    ui.checkAct->setEnabled(false);
    ui.runAct->setEnabled(false);
    ui.stopAct->setEnabled(false);
    ui.plotAct->setEnabled(false);
    ui.plotBoxAct->setEnabled(false);
    ui.plotEPSRshellAct->setEnabled(false);
    ui.plotOutputsMenu->setEnabled(false);
    ui.epsrManualAct->setEnabled(false);

}

void MainWindow::open()
{
    if (epsrBinDir_.isEmpty())
    {
        QMessageBox msgBox;
        msgBox.setText("The EPSR binary folder needs to be defined.");
        msgBox.exec();
        return;
    }

    QString newFileName;
    if (!epsrDir_.isEmpty())
    {
        newFileName = QFileDialog::getOpenFileName(this, "Choose EPSR .pro file", epsrDir_, tr(".EPSR.pro files (*.pro)"));
    }
    else
    newFileName = QFileDialog::getOpenFileName(this, "Choose EPSR .pro file", currentDir.path(), tr(".EPSR.pro files (*.pro)"));

    if (!newFileName.isEmpty())
    {
        //clear all tables
        reset();

        // set file directories
        projectName_ = QFileInfo(newFileName).baseName();
        workingDir_ = QFileInfo(newFileName).path()+"/";
        workingDir_ = QDir::toNativeSeparators(workingDir_);
        if (epsrDir_.isEmpty())
        {
            epsrDir_ = workingDir_.split(projectName_,QString::SkipEmptyParts).at(0);
            epsrDir_ = QDir::toNativeSeparators(epsrDir_);
        }

        ui.messagesLineEdit->setText("Please wait: opening project "+projectName_);

        messageText_ += "\n***************************************************************************\n";
        messageText_ += "Current EPSR project name is "+projectName_+"\n";
        messageText_ += "Current working directory is "+workingDir_+"\n";
        messageText_ += "Current EPSR binaries directory is "+epsrBinDir_+"\n";

        QDir::setCurrent(workingDir_);

        //read in EPSR pro file
        QFile file(newFileName);
        if(!file.open(QFile::ReadOnly | QFile::Text))
        {
            QMessageBox msgBox;
            msgBox.setText("Could not open EPSR pro file");
            msgBox.exec();
            return;
        }
        QTextStream stream(&file);
        QString line;
        QStringList dataLine;
        dataLine.clear();

        do
        {
            line = stream.readLine();
            dataLine = line.split(" ", QString::SkipEmptyParts);
            if (dataLine.count() != 0)
            {
                if (dataLine.at(0) == "mol")
                {
                    ui.molFileList->QListWidget::addItem(dataLine.at(1));
                    nMolFiles = ui.molFileList->count();
                    ui.atoFileTable->setRowCount(nMolFiles);
                    QStringList filename = dataLine.at(1).split(".", QString::SkipEmptyParts);
                    if (filename.at(1) == "ato")
                    {
                        ui.mixatoButton->setDisabled(true);
                        ui.loadBoxButton->setDisabled(true);
                    }
                    QTableWidgetItem *item = new QTableWidgetItem(filename.at(0)+".ato");
                    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
                    ui.atoFileTable->setItem(nMolFiles-1,0, item);
                    ui.atoFileTable->setItem(nMolFiles-1,2, new QTableWidgetItem(dataLine.at(2)));
                    ui.molFileList->setCurrentRow(nMolFiles-1);
                    ui.viewMolFileButton->setEnabled(true);
                    ui.removeMolFileButton->setEnabled(true);
                    ui.molFileTabWidget->setEnabled(true);
                    ui.updateMolFileButton->setEnabled(true);
                    ui.mixatoButton->setEnabled(true);
                    ui.addatoButton->setEnabled(true);
                    ui.loadBoxButton->setEnabled(true);
                }
                if (dataLine.at(0) == "boxAtoFileName")
                {
                    atoFileName_ = dataLine.at(1);
                    if (readAtoFileBoxDetails() == false)
                    {
                        ui.messagesLineEdit->setText("Simulation box corrupt");
                        return;
                    }
                    messageText_ += "Box .ato filename is "+atoFileName_+"\n";
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
                }
                if (dataLine.at(0) == "data")
                {
                    dataFileList.append(dataLine.at(2));
                    normalisationList.append(dataLine.at(3));
                    ui.exchangeableAtomsButton->setEnabled(true);
                    ui.makeWtsButton->setEnabled(true);
                    ui.setupEPSRButton->setEnabled(true); //a wts file is checked for on clicking this button
                }
                if (dataLine.at(0) == "wts")
                {
                    if (dataLine.count() == 2)
                    {
                        wtsFileList.append(dataLine.at(1));
                    }
                    else
                    {
                        wtsFileList.append(" ");
                    }
                }
                if (dataLine.at(0) == "EPSRinp")
                {
                    epsrInpFileName_ = dataLine.at(1)+".EPSR.inp";
                    if (readEPSRinpFile() == false)
                    {
                        return;
                    }
                    if (readEPSRpcofFile() == false)
                    {
                        return;
                    }
                    messageText_ += "EPSR inp filename is "+epsrInpFileName_+"\n";
                    ui.epsrInpFileName->setText(epsrInpFileName_);
                    updateInpFileTables();
                    updatePcofFileTables();
                    ui.updateInpPcofFilesButton->setEnabled(true);
                    ui.reloadEPSRinpButton->setEnabled(true);
                    ui.setupOutButton->setEnabled(true);
                    ui.setupPlotButton->setEnabled(true);
                    ui.applyOutputsButton->setEnabled(true);
                    ui.applyCommandButton->setEnabled(true);
                    ui.dlputilsOutCheckBox->setEnabled(true);
                    ui.deleteBoxAtoFileAct->setEnabled(true);
                    ui.plot1Button->setEnabled(true);
                    ui.plot2Button->setEnabled(true);
                    ui.removeComponentButton->setEnabled(false);
                    ui.dataFileBrowseButton->setEnabled(false);
                    ui.removeDataFileButton->setEnabled(false);
                    ui.checkAct->setEnabled(true);
                    ui.runAct->setEnabled(true);
                    ui.plotAct->setEnabled(true);
                    ui.plotEPSRshellAct->setEnabled(true);
                    ui.plotOutputsMenu->setEnabled(true);
                    ui.plot1Button->setEnabled(true);
                    ui.plot2Button->setEnabled(true);
                    ui.deleteEPSRinpFileAct->setEnabled(true);
                    //Activate available outputs list
                    getOutputType();
                    getOutputsRunning();
                    //open .bat file to check if dlputils line is in there
                    QString inpBaseFileName = epsrInpFileName_.split(".",QString::SkipEmptyParts).at(0);
#ifdef _WIN32
                    QFile batFile(workingDir_+"run"+inpBaseFileName+".bat");
#else
                    QFile batFile(workingDir_+"run"+inpBaseFileName+".sh");
#endif
                    //if bat file exists, make killepsr file in case EPSRgui crashed while running EPSR (this is then deleted on runEPSR())
                    if (batFile.exists() == true)
                    {
                        QFile file(workingDir_+"killepsr");
                        if(file.open(QFile::WriteOnly))
                        {
                            file.close();
                        }
                    }
                }
            }
        } while (!stream.atEnd());
        file.close();

        //re-write bat file if it exists to have correct epsrBinDir_, workingDir_ and format for operating system
        if (!epsrInpFileName_.isEmpty())
        {
            QString inpBaseFileName = epsrInpFileName_.split(".",QString::SkipEmptyParts).at(0);
#ifdef _WIN32
            QFile batFile(workingDir_+"run"+inpBaseFileName+".bat");
#else
            QFile batFile(workingDir_+"run"+inpBaseFileName+".sh");
#endif
            if (batFile.exists())
            {
                //make a new skeleton bat file
                if (batFile.open(QFile::WriteOnly | QFile::Text))
                {
                    QTextStream stream(&batFile);
                    QString baseFileName = epsrInpFileName_.split(".",QString::SkipEmptyParts).at(0);
#ifdef _WIN32
                    stream << "set EPSRbin=" << epsrBinDir_ << "\n"
                            << "set EPSRrun=" << workingDir_ << "\n"
                            << ":loop\n"
                            << "%EPSRbin%epsr.exe %EPSRrun% epsr " << baseFileName << "\n"
                            << "if not exist %EPSRrun%killepsr ( goto loop ) else del %EPSRrun%killepsr\n";
#else
                    stream << "export EPSRbin=" << epsrBinDir_ << "\n"
                            << "export EPSRrun=" << workingDir_ << "\n"
                            << "while :\n"
                            << "do\n"
                            << "  \"$EPSRbin\"'epsr' \"$EPSRrun\" epsr " << baseFileName << "\n"
                            << "  if ([ -e \"$EPSRrun\"killepsr ])\n"
                            << "  then break\n"
                            << "  fi\n"
                            << "done\n"
                            << "rm -r \"$EPSRrun\"killepsr\n";
#endif
                    batFile.close();
                }
                //add any outputs to skeleton bat file
                addOutputsToScript();
            }
        }

        // fill out datafile table
        refreshDataFileTable();

        //activate tabs
        ui.tabWidget->setEnabled(true);

        //activate menu options
        ui.saveAsAct->setEnabled(true);
        ui.epsrManualAct->setEnabled(true);
        ui.notesAct->setEnabled(true);

        //change window title to contain projectName
        this->setWindowTitle("EPSRgui: "+projectName_);

        //plot data if present
        if (!epsrInpFileName_.isEmpty())
        {
            QString atoBaseFileName = epsrInpFileName_.split(".",QString::SkipEmptyParts).at(0);
            if (QFile::exists(workingDir_+atoBaseFileName+".EPSR.u01")) //this was also included but not sure why: && !dataFileList.isEmpty())
            {
                plot1();
            }
            if (QFile::exists(workingDir_+atoBaseFileName+".EPSR.erg"))
            {
                plot2();
            }
        }
        messagesDialog.refreshMessages();
        ui.messagesLineEdit->setText("Project "+projectName_+" loaded");
    }
}

bool MainWindow::save()
{
    QString saveFileName = workingDir_+projectName_+".EPSR.pro";
    QFile file(saveFileName);

    if(!file.open(QFile::WriteOnly | QFile::Text))
    {
        QMessageBox msgBox;
        msgBox.setText("Could not save to "+projectName_+".EPSR.pro");
        msgBox.exec();
        return false;
    }

    QTextStream streamWrite(&file);

    //mol files and number of them in box
    if (ui.molFileList->count() != 0)
    {
        for (int i = 0; i < ui.molFileList->count(); i++)
        {
            streamWrite << "mol " << ui.molFileList->item(i)->text()
                        << " " << ui.atoFileTable->item(i,2)->text() << "\n";
        }
    }

    //box ato file details
    if (!atoFileName_.isEmpty())
    {
        streamWrite << "boxAtoFileName " << atoFileName_ << "\n";
    }

    //data and wts files
    if (dataFileList.count() != 0)
    {
        for (int i = 0; i < dataFileList.count(); i++)
        {
            streamWrite << "data 5 " << dataFileList.at(i) << " " << normalisationList.at(i) << "\n"; //nrtype no longer used, but to enable backwards compatibility of .pro file, keep a value here anyway
        }
        for (int i = 0; i < wtsFileList.count(); i++)
        {
            streamWrite << "wts " << wtsFileList.at(i) << "\n";
        }
    }

    //EPSR.inp and .pcof files
    if (!epsrInpFileName_.isEmpty())
    {
        QString atoBaseFileName = epsrInpFileName_.split(".",QString::SkipEmptyParts).at(0);
        streamWrite << "EPSRinp " << atoBaseFileName << "\n";
    }

    file.close();

    if (!epsrInpFileName_.isEmpty())
    {
        updateInpFile();
        updatePcofFile();
    }

    return true;
}

bool MainWindow::saveAs()
{
    CreateNewDialog createNewDialog(this);

    createNewDialog.show();
    createNewDialog.raise();
    createNewDialog.activateWindow();

    createNewDialog.setWindowTitle("Save As...");

    newDialog = createNewDialog.exec();

    if (newDialog == CreateNewDialog::Accepted)
    {
        //set file directories
        QString projectNameCopy = createNewDialog.getEPSRname();
        QString epsrDirCopy = createNewDialog.getPrefDir();
        QString workingDirCopy = (epsrDirCopy+"/"+projectNameCopy+"/");
        workingDirCopy = QDir::toNativeSeparators(workingDirCopy);
        QDir workingDir(workingDir_);

        ui.messagesLineEdit->setText("Please wait: saving to project "+projectNameCopy);
        ui.tabWidget->setDisabled(true);

        //make new directory and make a list of everything in the current directory
        QDir().mkdir(workingDirCopy);
        QStringList fileList = workingDir.entryList(QDir::Files);

        //copy everything into new folder
        for (int i = 0; i < fileList.count(); i++)
        {
            QFile::copy(workingDir_+fileList.at(i), workingDirCopy+fileList.at(i));
        }

        //rename everything with original atoFileName_ in title
        QDir newDir(workingDirCopy);
        QStringList newfileList = newDir.entryList(QDir::Files);
        for (int i = 0; i < newfileList.count(); i++)
        {
            QString oldfilename = newfileList.at(i);
            QString newfilename = newfileList.at(i);
            newfilename.replace(projectName_+"box", projectNameCopy+"box");
            QFile::rename(workingDirCopy+oldfilename, workingDirCopy+newfilename);
        }

        //delete original .EPSR.pro file
        QString epsrProFile = workingDirCopy+projectName_+".EPSR.pro";
        QFile filedelete(epsrProFile);
        filedelete.remove();

        //make new .EPSR.pro file and write to it
        QString epsrProFileCopy = workingDirCopy+projectNameCopy+".EPSR.pro";
        QFile file(epsrProFileCopy);

        if(!file.open(QFile::WriteOnly | QFile::Text))
        {
            QMessageBox msgBox;
            msgBox.setText("Could not save to "+projectNameCopy+".EPSR.pro");
            msgBox.exec();
            return false;
        }

        QTextStream streamWrite(&file);

        //mol files and number of them in box
        if (ui.molFileList->count() != 0)
        {
            for (int i = 0; i < ui.molFileList->count(); i++)
            {
                streamWrite << "mol " << ui.molFileList->item(i)->text()
                            << " " << ui.atoFileTable->item(i,2)->text() << "\n";
            }
        }

        //box ato file details
        if (!atoFileName_.isEmpty())
        {
            streamWrite << "boxAtoFileName " << projectNameCopy+"box.ato" << "\n";
            ui.boxAtoLabel->setText(projectNameCopy+"box.ato");
        }

        //data and wts files
        if (ui.dataFileTable->rowCount() != 0)
        {
            for (int i = 0; i < ui.dataFileTable->rowCount(); i++)
            {
                streamWrite << "data 5 " << ui.dataFileTable->item(i,0)->text() << " " << ui.dataFileTable->item(i,1)->text() << "\n";
            }
            for (int i = 0; i < wtsFileList.count(); i++)
            {
                streamWrite << "wts " << ui.dataFileTable->item(i,2)->text() << "\n";
            }
        }

        //EPSR.inp file
        if (!epsrInpFileName_.isEmpty())
        {
            streamWrite << "EPSRinp " << projectNameCopy+"box" << "\n";
        }

        //dlputils
        if (ui.dlputilsOutCheckBox->isChecked())
        {
            streamWrite << "dlputils\n";
        }
        file.close();

        //change fnameato in NWTS.dat and XWTS.dat
        if (!wtsFileList.isEmpty())
        {
            for (int i = 0; i < wtsFileList.count(); i++)
            {
                QString wtsFileName;
                QString wtsFileType = wtsFileList.at(i).split(".",QString::SkipEmptyParts).at(1);
                if (wtsFileType == "XWTS")
                {
                    wtsFileName = wtsFileList.at(i).split(".",QString::SkipEmptyParts).at(0)+".XWTS.dat";
                }
                else
                {
                    wtsFileName = wtsFileList.at(i).split(".",QString::SkipEmptyParts).at(0)+".NWTS.dat";
                }

                QFile filewts(workingDirCopy+wtsFileName);
                if(!filewts.open(QFile::ReadWrite | QFile::Text))
                {
                    QMessageBox msgBox;
                    msgBox.setText("Could not open .wts file");
                    msgBox.exec();
                    return false;
                }

                QTextStream streamwts(&filewts);
                QString linewts;
                QStringList dataLinewts;
                dataLinewts.clear();
                QString originalwts;

                while (!streamwts.atEnd())
                {
                    linewts = streamwts.readLine();
                    dataLinewts = linewts.split("  ", QString::SkipEmptyParts);
                    originalwts.append(linewts+"\n");
                    if (dataLinewts.count()!=0)
                    {
                        if (dataLinewts.at(0) == "fnameato")
                        {
                            originalwts.remove(linewts+"\n");
                            originalwts.append("fnameato    "+projectNameCopy+"box.ato               Name of .ato file\n");
                        }
                    }
                }
                filewts.resize(0);
                streamwts << originalwts;
                filewts.close();
            }
        }

        //edit EPSR.inp file so first line, fnameato and fnamepcof are updated
        if (!epsrInpFileName_.isEmpty())
        {
            QFile fileRead(workingDir_+projectName_+"box.EPSR.inp");
            if(!fileRead.open(QFile::ReadOnly | QFile::Text))
            {
                QMessageBox msgBox;
                msgBox.setText("Could not open .EPSR.inp file to copy");
                msgBox.exec();
                return false;
            }

            QFile fileWrite(workingDirCopy+projectNameCopy+"box.EPSR.inp");
            if(!fileWrite.open(QFile::WriteOnly | QFile::Text))
            {
                QMessageBox msgBox;
                msgBox.setText("Could not open new .EPSR.inp file to write to");
                msgBox.exec();
                return false;
            }

            QTextStream streamRead(&fileRead);
            QTextStream streamWrite(&fileWrite);
            QString line;
            QStringList dataLine;
            dataLine.clear();
            QString original;

            line = streamRead.readLine();
            streamWrite << projectNameCopy+"box.EPSR               Title of this file\n";

            while (!streamRead.atEnd())
            {
                line = streamRead.readLine();
                dataLine = line.split("   ", QString::SkipEmptyParts);
                original.append(line+"\n");
                if (dataLine.count()!=0)
                {
                    if (dataLine.at(0) == "fnameato")
                    {
                        original.remove(line+"\n");
                        original.append("fnameato    "+projectNameCopy+"box.ato               Name of .ato file\n");
                    }
                    if (dataLine.at(0) == "fnamepcof")
                    {
                        original.remove(line+"\n");
                        original.append("fnamepcof   "+projectNameCopy+"box.pcof               Name of potential coefficients file.\n");
                    }
                }
            }
            fileWrite.resize(0);
            streamWrite << original;
            fileWrite.close();
            fileRead.close();
        }

        //update EPSR script file - this only works if box is projectName_+"box" ***********************************
#ifdef _WIN32
        QFile batFile(workingDirCopy+"run"+projectNameCopy+"box.bat");
#else
        QFile batFile(workingDirCopy+"run"+projectNameCopy+"box.sh");
#endif
        if (batFile.exists() == true)
        {
            if (!batFile.open(QFile::ReadWrite | QFile::Text))
            {
                QMessageBox msgBox;
                msgBox.setText("Could not open script file to edit.");
                msgBox.exec();
                return false;
            }

            QTextStream stream(&batFile);
            QString line;
            QString original;

            do {
                line = stream.readLine();
                line.replace(projectName_, projectNameCopy);
                original.append(line+"\n");
            } while (!line.isNull());
            batFile.resize(0);
            stream << original;
            batFile.close();
        }

        // set file directories etc              
        projectName_ = projectNameCopy;
        messageText_ += "Current EPSR project name is "+projectName_+"\n";
        workingDir_ = workingDirCopy;
        workingDir_ = QDir::toNativeSeparators(workingDir_);
        messageText_ += "Current working directory is "+workingDir_+"\n";
        QDir::setCurrent(workingDir_);
        epsrDir_ = epsrDirCopy;
        messageText_ += "Current EPSR binaries directory is "+epsrBinDir_+"\n";
        if (!atoFileName_.isEmpty())
        {
            atoFileName_ = projectNameCopy+"box.ato";
        }
        if (!epsrInpFileName_.isEmpty())
        {
            epsrInpFileName_ = projectNameCopy+"box.EPSR.inp";
            ui.epsrInpFileName->setText(epsrInpFileName_);
            readEPSRinpFile();
            updateInpFileTables();
        }

//        //ensure current settings are saved to .pro file - this is already done when writing .pro file
//        save();

        //change window title to contain projectName
        this->setWindowTitle("EPSRgui: "+projectName_);
        ui.tabWidget->setDisabled(false);

        messagesDialog.refreshMessages();
        ui.messagesLineEdit->setText("EPSR project saved to "+workingDirCopy);
    }
    return false;
}

bool MainWindow::saveCopy()
{
    CreateNewDialog createNewDialog(this);

    createNewDialog.show();
    createNewDialog.raise();
    createNewDialog.activateWindow();

    newDialog = createNewDialog.exec();

    if (newDialog == CreateNewDialog::Accepted)
    {
        //set file directories
        QString projectNameCopy = createNewDialog.getEPSRname();
        QString epsrDirCopy = createNewDialog.getPrefDir();
        QString workingDirCopy = (epsrDirCopy+"/"+projectNameCopy+"/");
        workingDirCopy = QDir::toNativeSeparators(workingDirCopy);
        QDir workingDir(workingDir_);

        //make new directory and make a list of everything in the current directory
        QDir().mkdir(workingDirCopy);
        QStringList fileList = workingDir.entryList(QDir::Files);

        //copy everything into new folder
        for (int i = 0; i < fileList.count(); i++)
        {
            QFile::copy(workingDir_+fileList.at(i), workingDirCopy+fileList.at(i));
        }

        //rename everything with original atoFileName_ in title
        QDir newDir(workingDirCopy);
        QStringList newfileList = newDir.entryList(QDir::Files);
        for (int i = 0; i < newfileList.count(); i++)
        {
            QString oldfilename = newfileList.at(i);
            QString newfilename = newfileList.at(i);
            newfilename.replace(projectName_+"box", projectNameCopy+"box");
            QFile::rename(workingDirCopy+oldfilename, workingDirCopy+newfilename);
        }

        //delete original .EPSR.pro file
        QString epsrProFile = workingDirCopy+projectName_+".EPSR.pro";
        QFile filedelete(epsrProFile);
        filedelete.remove();

        //make new .EPSR.pro file
        QString epsrProFileCopy = workingDirCopy+projectNameCopy+".EPSR.pro";
        QFile file(epsrProFileCopy);

        if(!file.open(QFile::WriteOnly | QFile::Text))
        {
            QMessageBox msgBox;
            msgBox.setText("Could not save to "+projectNameCopy+".EPSR.pro");
            msgBox.exec();
            return false;
        }

        QTextStream streamWrite(&file);

        //mol files and number of them in box
        if (ui.molFileList->count() != 0)
        {
            for (int i = 0; i < ui.molFileList->count(); i++)
            {
                streamWrite << "mol " << ui.molFileList->item(i)->text()
                            << " " << ui.atoFileTable->item(i,2)->text() << "\n";
            }
        }

        //box ato file details
        if (!atoFileName_.isEmpty())
        {
            streamWrite << "boxAtoFileName " << projectNameCopy+"box.ato" << "\n";
            streamWrite << "numberDensity " << ui.numberDensityLineEdit->text() << "\n";
            streamWrite << "temperature " << ui.temperatureLineEdit->text() << "\n";
            streamWrite << "vibtemp " << ui.vibtempLineEdit->text() << "\n";
            streamWrite << "angtemp " << ui.angtempLineEdit->text() << "\n";
            streamWrite << "dihtemp " << ui.dihtempLineEdit->text() << "\n";
            streamWrite << "ecoredcore " << ui.ecoreLineEdit->text() << " " << ui.dcoreLineEdit->text() << "\n";

            //fmole iterations
            streamWrite << "fmoleIter " << ui.fmoleLineEdit->text() << "\n";
        }

        //data and wts files
        if (ui.dataFileTable->rowCount() != 0)
        {
            for (int i = 0; i < ui.dataFileTable->rowCount(); i++)
            {
                streamWrite << "data 5 " << ui.dataFileTable->item(i,0)->text() << " " << ui.dataFileTable->item(i,1)->text() << "\n";
            }
            for (int i = 0; i < wtsFileList.count(); i++)
            {
                streamWrite << "wts " << ui.dataFileTable->item(i,2)->text() << "\n";
            }
        }

        //change fnameato in NWTS.dat and XWTS.dat
        if (!wtsFileList.isEmpty())
        {
            for (int i = 0; i < wtsFileList.count(); i++)
            {
                QString wtsFileName;
                QString wtsFileType = wtsFileList.at(i).split(".",QString::SkipEmptyParts).at(1);
                if (wtsFileType == "XWTS")
                {
                    wtsFileName = wtsFileList.at(i).split(".",QString::SkipEmptyParts).at(0)+".XWTS.dat";
                }
                else
                {
                    wtsFileName = wtsFileList.at(i).split(".",QString::SkipEmptyParts).at(0)+".NWTS.dat";
                }

                QFile filewts(workingDirCopy+wtsFileName);
                if(!filewts.open(QFile::ReadWrite | QFile::Text))
                {
                    QMessageBox msgBox;
                    msgBox.setText("Could not open .wts file");
                    msgBox.exec();
                    return false;
                }

                QTextStream streamwts(&filewts);
                QString linewts;
                QStringList dataLinewts;
                dataLinewts.clear();
                QString originalwts;

                while (!streamwts.atEnd())
                {
                    linewts = streamwts.readLine();
                    dataLinewts = linewts.split("  ", QString::SkipEmptyParts);
                    originalwts.append(linewts+"\n");
                    if (dataLinewts.count()!=0)
                    {
                        if (dataLinewts.at(0) == "fnameato")
                        {
                            originalwts.remove(linewts+"\n");
                            originalwts.append("fnameato    "+projectNameCopy+"box.ato               Name of .ato file\n");
                        }
                    }
                }

                filewts.resize(0);
                streamwts << originalwts;
                filewts.close();
            }
        }

        //EPSR.inp and .pcof files
        if (!epsrInpFileName_.isEmpty())
        {
            streamWrite << "EPSRinp " << projectNameCopy+"box" << "\n";
        }

        file.close();

        //edit EPSR.inp file so first line, fnameato and fnamepcof are updated
        if (!epsrInpFileName_.isEmpty())
        {
            QFile fileRead(workingDir_+projectName_+"box.EPSR.inp");
            if(!fileRead.open(QFile::ReadOnly | QFile::Text))
            {
                QMessageBox msgBox;
                msgBox.setText("Could not open .EPSR.inp file to copy");
                msgBox.exec();
                return false;
            }

            QFile fileWrite(workingDirCopy+projectNameCopy+"box.EPSR.inp");
            if(!fileWrite.open(QFile::WriteOnly | QFile::Text))
            {
                QMessageBox msgBox;
                msgBox.setText("Could not open new .EPSR.inp file to write to");
                msgBox.exec();
                return false;
            }

            QTextStream streamRead(&fileRead);
            QTextStream streamWrite(&fileWrite);
            QString line;
            QStringList dataLine;
            dataLine.clear();
            QString original;

            line = streamRead.readLine();
            streamWrite << projectNameCopy+"box.EPSR               Title of this file\n";

            while (!streamRead.atEnd())
            {
                line = streamRead.readLine();
                dataLine = line.split("   ", QString::SkipEmptyParts);
                original.append(line+"\n");
                if (dataLine.count()!=0)
                {
                    if (dataLine.at(0) == "fnameato")
                    {
                        original.remove(line+"\n");
                        original.append("fnameato    "+projectNameCopy+"box.ato               Name of .ato file\n");
                    }
                    if (dataLine.at(0) == "fnamepcof")
                    {
                        original.remove(line+"\n");
                        original.append("fnamepcof   "+projectNameCopy+"box.pcof               Name of potential coefficients file.\n");
                    }
                }
            }

            fileWrite.resize(0);
            streamWrite << original;
            fileWrite.close();
            fileRead.close();
        }

        //update EPSR script file - this only works if box is projectName_+"box" ***********************************
#ifdef _WIN32
        QFile batFile(workingDirCopy+"run"+projectNameCopy+"box.bat");
#else
        QFile batFile(workingDirCopy+"run"+projectNameCopy+"box.sh");
#endif
        if (batFile.exists() == true)
        {
            if (!batFile.open(QFile::ReadWrite | QFile::Text))
            {
                QMessageBox msgBox;
                msgBox.setText("Could not open script file to edit.");
                msgBox.exec();
                return false;
            }

            QTextStream stream(&batFile);
            QString line;
            QString original;

            do {
                line = stream.readLine();
                line.replace(projectName_, projectNameCopy);
                original.append(line+"\n");
            } while (!line.isNull());
            batFile.resize(0);
            stream << original;
            batFile.close();
        }

        messageText_ += "Copied EPSR project to "+workingDirCopy+"\n";
        messagesDialog.refreshMessages();
        ui.messagesLineEdit->setText("Copied current EPSR project to "+workingDirCopy);
    }
    return false;
}

void MainWindow::import()
{
    if (epsrBinDir_.isEmpty())
    {
        QMessageBox msgBox;
        msgBox.setText("The EPSR binaries directory needs to be defined.");
        msgBox.exec();
        return;
    }

    ImportDialog importDialog(this);

    importDialog.setModal(true);
    importDialog.show();
    importDialog.raise();
    importDialog.activateWindow();

    newImportDialog = importDialog.exec();

    if (newImportDialog == ImportDialog::Accepted)
    {
        reset();

        //set file directories
        projectName_ = importDialog.getProjectName();
        workingDir_ = (epsrDir_+projectName_+"/");
        workingDir_ = QDir::toNativeSeparators(workingDir_);
        if (epsrDir_.isEmpty())
        {
            epsrDir_ = workingDir_.split(projectName_,QString::SkipEmptyParts).at(0);
            epsrDir_ = QDir::toNativeSeparators(epsrDir_);
        }

        ui.messagesLineEdit->setText("Please wait: importing project "+projectName_);
        ui.tabWidget->setDisabled(true);

        messageText_ += "\n***************************************************************************\n";
        messageText_ += "Imported existing simulation "+projectName_+" into EPSRgui\n";
        messageText_ += "Current EPSR project name is "+projectName_+"\n";
        messageText_ += "Current working directory is "+workingDir_+"\n";
        messageText_ += "Current EPSR binaries directory is "+epsrBinDir_+"\n";

        QDir::setCurrent(workingDir_);

        //copy important EPSR files into working directory
        QString epsrStartupDir = epsrBinDir_.split("bin", QString::SkipEmptyParts).at(0);
        epsrStartupDir = epsrStartupDir+"startup/";
        epsrStartupDir = QDir::toNativeSeparators(epsrStartupDir);
        QFile::copy(epsrStartupDir+"f0_WaasKirf.dat", workingDir_+"f0_WaasKirf.dat");
        QFile::copy(epsrStartupDir+"plot_defaults.txt", workingDir_+"plot_defaults.txt");
        QFile::copy(epsrStartupDir+"vanderWaalsRadii.txt", workingDir_+"vanderWaalsRadii.txt");
        QFile::copy(epsrStartupDir+"gnubonds.txt", workingDir_+"gnubonds.txt");
        QFile::copy(epsrStartupDir+"gnuatoms.txt", workingDir_+"gnuatoms.txt");
#ifdef _WIN32
        QFile::copy(epsrStartupDir+"epsr.bat", workingDir_+"epsr.bat");
        QFile::copy(epsrStartupDir+"system_commands_windows.txt", workingDir_+"system_commands.txt");
#else
        QFile::copy(epsrStartupDir+"epsr", workingDir_+"epsr");
        QFile::copy(epsrStartupDir+"system_commands_linux.txt", workingDir_+"system_commands.txt");
#endif

        //read in EPSR pro file
        QString epsrProFile = workingDir_+projectName_+".EPSR.pro";
        epsrProFile = QDir::toNativeSeparators(epsrProFile);
        QFile file(epsrProFile);
        if(!file.open(QFile::ReadOnly | QFile::Text))
        {
            QMessageBox msgBox;
            msgBox.setText("Could not open EPSR pro file");
            msgBox.exec();
            return;
        }
        QTextStream stream(&file);
        QString line;
        QStringList dataLine;
        dataLine.clear();

        QStringList molFileList;
        QStringList numberMolList;
        molFileList.clear();
        numberMolList.clear();
        dataFileList.clear();
        wtsFileList.clear();
        normalisationList.clear();

        do
        {
            line = stream.readLine();
            dataLine = line.split(" ", QString::SkipEmptyParts);
            if (dataLine.count() != 0)
            {
                if (dataLine.at(0) == "mol")
                {
                    molFileList.append(dataLine.at(1));
                    numberMolList.append(dataLine.at(2));
                }
                if (dataLine.at(0) == "boxAtoFileName")
                {
                    atoFileName_ = dataLine.at(1);
                    messageText_ += "Box .ato filename is "+atoFileName_+"\n";
                    ui.boxAtoLabel->setText(atoFileName_);
                }
                if (dataLine.at(0) == "data")
                {
                    dataFileList.append(dataLine.at(2));
                    normalisationList.append(dataLine.at(3));
                    ui.exchangeableAtomsButton->setEnabled(true);
                    ui.makeWtsButton->setEnabled(true);
                }
                if (dataLine.at(0) == "wts")
                {
                    if (dataLine.count() == 2)
                    {
                        wtsFileList.append(dataLine.at(1));
                        ui.setupEPSRButton->setEnabled(true);
                    }
                    else
                    {
                        wtsFileList.append(" ");
                    }
                }
                if (dataLine.at(0) == "EPSRinp")
                {
                    epsrInpFileName_ = dataLine.at(1)+".EPSR.inp";
                    messageText_ += "EPSR inp filename is "+epsrInpFileName_+"\n";
                    ui.epsrInpFileName->setText(epsrInpFileName_);
                }
            }
        } while (!stream.atEnd());
        file.close();

        QDir::setCurrent(workingDir_);

        QStringList firstAtomTypes;
        firstAtomTypes.clear();
        QList<int> numberFirstAtomTypes;
        numberFirstAtomTypes.clear();

        //fill out components tables
        if (molFileList.count() != 0)
        {
            nMolFiles = molFileList.count();
            ui.atoFileTable->setRowCount(nMolFiles);
            for (int i = 0; i < molFileList.count(); i++)
            {
                ui.molFileList->QListWidget::addItem(molFileList.at(i));
                QStringList filename = molFileList.at(i).split(".", QString::SkipEmptyParts);
                QTableWidgetItem *item = new QTableWidgetItem(filename.at(0)+".ato");
                item->setFlags(item->flags() & ~Qt::ItemIsEditable);
                ui.atoFileTable->setItem(i,0, item);
                ui.atoFileTable->setItem(i,2, new QTableWidgetItem(numberMolList.at(i)));
                if (filename.at(1) == "ato")
                {
                    ui.mixatoButton->setDisabled(true);
                    ui.loadBoxButton->setDisabled(true);
                }
#ifdef _WIN32
                processEPSR_.start(epsrBinDir_+"fmole.exe", QStringList() << workingDir_ << "fmole" << filename.at(0) << "0");
#else
                processEPSR_.start(epsrBinDir_+"fmole", QStringList() << workingDir_ << "fmole" << filename.at(0) << "0");
#endif
                if (!processEPSR_.waitForStarted()) return;
                if (!processEPSR_.waitForFinished(60000)) return;
                ui.molFileList->setCurrentRow(i);
                firstAtomTypes.append(ui.molLJTable->item(0,0)->text());
                numberFirstAtomTypes.append(numberComponentAtomLabels.at(0));
            }

            ui.viewMolFileButton->setEnabled(true);
            ui.removeMolFileButton->setEnabled(true);
            ui.molFileTabWidget->setEnabled(true);
            ui.updateMolFileButton->setEnabled(true);
            ui.mixatoButton->setEnabled(true);
            ui.addatoButton->setEnabled(true);
            ui.loadBoxButton->setEnabled(true);
        }

        //get box details
        if (!atoFileName_.isEmpty())
        {
            QString atoBaseFileName = atoFileName_.split(".", QString::SkipEmptyParts).at(0);
#ifdef _WIN32
            processEPSR_.start(epsrBinDir_+"fmole.exe", QStringList() << workingDir_ << "fmole" << atoBaseFileName << "0");
#else
            processEPSR_.start(epsrBinDir_+"fmole", QStringList() << workingDir_ << "fmole" << atoBaseFileName << "0");
#endif
            if (!processEPSR_.waitForStarted()) return;
            if (!processEPSR_.waitForFinished(60000)) return;

            //populate atoTable with correct number of each component in box
            QFile fileato(workingDir_+atoFileName_);
            if(!fileato.open(QFile::ReadOnly | QFile::Text))
            {
                QMessageBox msgBox;
                msgBox.setText("Could not open .ato file.");
                msgBox.exec();
                return;
            }
            QTextStream streamato(&fileato);
            QString lineato;
            QStringList dataLineato;
            dataLineato.clear();

            QVector<int> numberComponent;
            numberComponent.clear();
            numberComponent.resize(molFileList.count());
            QRegExp atomLabelrx(" ([A-Z][A-Za-z0-9 ]{2})   ([0-9 ]{1,4})      0");
            do {
                lineato = streamato.readLine();
                if (atomLabelrx.exactMatch(lineato))
                {
                    dataLineato = lineato.split("  ", QString::SkipEmptyParts);
                    QString trimmeddataLine = dataLineato.at(0).trimmed();
                    for (int i = 0; i < firstAtomTypes.count(); i++)
                    {
                        if (trimmeddataLine == firstAtomTypes.at(i))
                        {
                            //add 1 to label 'counter' each time find each label (therefore order is the same as ljAtoms)
                            numberComponent.operator [](i)++;
                        }
                    }
                }
            } while (!lineato.isNull());
            fileato.close();

            //for each mol file, divide numberInstances by numberFirstAtomTypes
            for (int i = 0; i < nMolFiles; i++)
            {
                int nComponents = numberComponent.at(i)/numberFirstAtomTypes.at(i);
                QString nComponentsStr = QString::number(nComponents);
                ui.atoFileTable->setItem(i,2, new QTableWidgetItem(nComponentsStr));
            }

            readAtoFileBoxDetails();
            checkBoxCharge();
            ui.randomiseButton->setEnabled(true);
            ui.plotBoxAct->setEnabled(true);
            ui.boxCompositionButton->setEnabled(true);
            ui.updateAtoFileButton->setEnabled(true);
            ui.fmoleButton->setEnabled(true);
            ui.atoEPSRButton->setEnabled(true);
            ui.dataFileBrowseButton->setEnabled(true);
            ui.removeDataFileButton->setEnabled(true);
            ui.deleteBoxAtoFileAct->setEnabled(true);
        }

        // fill out wts table
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
            ui.dataFileTable->setCurrentCell(dataFileList.count()-1,0);
        }
        if (!wtsFileList.isEmpty())
        {
            for (int i = 0; i < dataFileList.count(); i++)
            {
                ui.dataFileTable->setItem(i,3, new QTableWidgetItem(wtsFileList.at(i)));
            }
        }

        //fill out input file tab
        if (!epsrInpFileName_.isEmpty())
        {
            //find pcof file - if can't find one with correct name, copy to correct name
            QString atobaseFileName = atoFileName_.split(".",QString::SkipEmptyParts).at(0);
            QString pcofFileName = workingDir_+atobaseFileName+".pcof";
            QFile file(pcofFileName);
            if (!file.exists())
            {
                QDir dir;
                dir.setPath(workingDir_);
                QStringList pcofFilter;
                pcofFilter << "*.pcof";
                QStringList pcofFiles = dir.entryList(pcofFilter, QDir::Files);
                if (pcofFiles.count() == 0)
                {
                    QMessageBox msgBox;
                    msgBox.setText("Could not find a .pcof file.");
                    msgBox.exec();
                    return;
                }
                else
                if (pcofFiles.count() >= 2)
                {
                    QMessageBox msgBox;
                    msgBox.setText("Multiple .pcof files found in folder.");
                    msgBox.exec();
                    return;
                }
                else
                {
                    QFile::rename(workingDir_+pcofFiles.at(0), pcofFileName);
                    QFile::remove(workingDir_+pcofFiles.at(0));
                }

                //write name of pcof file to EPSR.inp file
                QFile epsrfile(workingDir_+epsrInpFileName_);
                QFile fileWrite(workingDir_+"temp.txt");
                if (!epsrfile.open(QFile::ReadOnly | QFile::Text))
                {
                    QMessageBox msgBox;
                    msgBox.setText("Could not open .EPSR.inp file.");
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
                QTextStream streamepsr(&epsrfile);
                QTextStream streamWrite(&fileWrite);
                QString lineepsr;
                QStringList dataLineepsr;
                dataLineepsr.clear();

                do
                {
                    lineepsr = streamepsr.readLine();
                    if (lineepsr.contains("fnamepcof"))
                    {
                        lineepsr.clear();
                        lineepsr = "fnamepcof   "+atobaseFileName+".pcof               Name of potential coefficients file.";
                    }
                    streamWrite << lineepsr << "\n";
                } while (!lineepsr.isNull());

                fileWrite.close();
                epsrfile.close();
                QFile::remove(workingDir_+epsrInpFileName_);
                QFile::rename(workingDir_+"temp.txt", workingDir_+epsrInpFileName_);
            }

            //run setup epsr fromat correctly
            QDir::setCurrent(workingDir_);
            QString baseFileName = epsrInpFileName_.split(".", QString::SkipEmptyParts).at(0);
#ifdef _WIN32
            processEPSR_.start(epsrBinDir_+"upset.exe", QStringList() << workingDir_ << "upset" << "epsr" << baseFileName);
#else
            processEPSR_.start(epsrBinDir_+"upset", QStringList() << workingDir_ << "upset" << "epsr" << baseFileName);
#endif
            if (!processEPSR_.waitForStarted()) return;
            processEPSR_.write("e\n");
            processEPSR_.write("\n");
             processEPSR_.write("y\n");
            if (!processEPSR_.waitForFinished(60000)) return;

            //read in files
            readEPSRinpFile();
            updateInpFileTables();
            readEPSRpcofFile();
            updatePcofFileTables();
            ui.updateInpPcofFilesButton->setEnabled(true);
            ui.reloadEPSRinpButton->setEnabled(true);
            ui.setupOutButton->setEnabled(true);
            ui.setupPlotButton->setEnabled(true);
            ui.applyOutputsButton->setEnabled(true);
            ui.applyCommandButton->setEnabled(true);
            ui.dlputilsOutCheckBox->setEnabled(true);
            ui.deleteBoxAtoFileAct->setEnabled(true);
            ui.plot1Button->setEnabled(true);
            ui.plot2Button->setEnabled(true);
            ui.dataFileBrowseButton->setDisabled(true);
            ui.removeDataFileButton->setEnabled(false);
            ui.checkAct->setEnabled(true);
            ui.runAct->setEnabled(true);
            ui.plotAct->setEnabled(true);
            ui.plotEPSRshellAct->setEnabled(true);
            ui.plotOutputsMenu->setEnabled(true);
            ui.plot1Button->setEnabled(true);
            ui.plot2Button->setEnabled(true);
            ui.deleteEPSRinpFileAct->setEnabled(true);

            //Activate available outputs list
            getOutputType();

            //open .bat file to check which analyses are being run at the same time as epsr
            QString scriptFile = importDialog.getScriptFile();
            if (!scriptFile.isEmpty())
            {
                QFile batFile(workingDir_+scriptFile);
                QTextStream batstream(&batFile);
                QString batline;
                QStringList batdataLine;
                batdataLine.clear();
                if (batFile.exists() == false)
                {
                    QMessageBox msgBox;
                    msgBox.setText("Could not find script file.");
                    msgBox.exec();
                    return;
                }

                if (!batFile.open(QFile::ReadOnly | QFile::Text))
                    {
                        QMessageBox msgBox;
                        msgBox.setText("Could not open script file.");
                        msgBox.exec();
                        return;
                    }

                do {
                    batline = batstream.readLine();
                    if (batline.contains("chains")
                            || batline.contains("clusters")
                            || batline.contains("coord")
                            || batline.contains("mapgr")
                            || batline.contains("partials")
                            || batline.contains("rings")
                            || batline.contains("sdf")
                            || batline.contains("sdfcube")
                            || batline.contains("sharm")
                            || batline.contains("torangles")
                            || batline.contains("triangles")
                            || batline.contains("voids"))
                    {
                        if (!batline.contains("#"))
                        {
                            batdataLine = batline.split(" ", QString::SkipEmptyParts);
                            ui.runOutEPSRList->addItem(batdataLine.at(0)+": "+batdataLine.at(1));
                        }
                    }
                    if (batline.contains("writexyz"))
                    {
                        if (!batline.contains("#"))
                        {
                            ui.dlputilsOutCheckBox->setChecked(true);
                        }
                    }
                }while (!batline.isNull());
                batFile.close();
            }
        }

        //activate tabs
        ui.tabWidget->setEnabled(true);

        //activate menu options
        ui.saveAsAct->setEnabled(true);
        ui.epsrManualAct->setEnabled(true);

        //change window title to contain projectName
        this->setWindowTitle("EPSRgui: "+projectName_);

        //plot data if present
        if (!epsrInpFileName_.isEmpty())
        {
            QString atoBaseFileName = epsrInpFileName_.split(".",QString::SkipEmptyParts).at(0);
            if (QFile::exists(atoBaseFileName+".EPSR.u01")) //this was also included but not sure why: && !dataFileList.isEmpty())
            {
                plot1();
            }
            if (QFile::exists(atoBaseFileName+".EPSR.erg"))
            {
                plot2();
            }

            //make run script file and include any analyses
            QString baseFileName = epsrInpFileName_.split(".",QString::SkipEmptyParts).at(0);

#ifdef _WIN32
            QFile writebatFile(workingDir_+"run"+baseFileName+".bat");
#else
            QFile writebatFile(workingDir_+"run"+baseFileName+".sh");
#endif

            if(!writebatFile.open(QFile::WriteOnly | QFile::Text))
            {
                QMessageBox msgBox;
                msgBox.setText("Could not write to script file.");
                msgBox.exec();
                return;
            }

            //write script file to run epsr
            QTextStream writebatstream(&writebatFile);
#ifdef _WIN32
            writebatstream << "set EPSRbin=" << epsrBinDir_ << "\n"
                    << "set EPSRrun=" << workingDir_ << "\n"
                    << ":loop\n"
                    << "%EPSRbin%epsr.exe %EPSRrun% epsr " << baseFileName << "\n"
                    << "if not exist %EPSRrun%killepsr ( goto loop ) else del %EPSRrun%killepsr\n";
#else
            writebatstream << "export EPSRbin=" << epsrBinDir_ << "\n"
                    << "export EPSRrun=" << workingDir_ << "\n"
                    << "while :\n"
                    << "do\n"
                    << "  \"$EPSRbin\"'epsr' \"$EPSRrun\" epsr " << baseFileName << "\n"
                    << "  if ([ -e " << workingDir_ << "killepsr ])\n"
                    << "  then break\n"
                    << "  fi\n"
                    << "done\n"
                    << "rm -r " << workingDir_ << "killepsr\n";
#endif
            writebatFile.close();

            addOutputsToScript();
        }

        save();

        ui.tabWidget->setDisabled(false);
        messagesDialog.refreshMessages();
        ui.messagesLineEdit->setText("New project "+projectName_+" imported");
    }
}

void MainWindow::runEPSRonce()
{
    runEPSR();
    stopEPSR();
}

void MainWindow::runEPSR()
{
    //remove killepsr if it exists
    QFile file(workingDir_+"killepsr");
    if (file.exists())
    {
            QFile::remove(workingDir_+"killepsr");
    }

    updateInpFile();
    updatePcofFile();

    QString baseFileName = epsrInpFileName_.split(".",QString::SkipEmptyParts).at(0);

#ifdef _WIN32
    QFile batFile(workingDir_+"run"+baseFileName+".bat");
#else
    QFile batFile(workingDir_+"run"+baseFileName+".sh");
#endif

    // if script file already exists, don't overwrite it
    if (!batFile.exists())
    {
        if(!batFile.open(QFile::WriteOnly | QFile::Text))
        {
            QMessageBox msgBox;
            msgBox.setText("Could not open script file.");
            msgBox.exec();
            return;
        }

        //write script file to run epsr
        QTextStream stream(&batFile);
#ifdef _WIN32
        stream << "set EPSRbin=" << epsrBinDir_ << "\n"
                << "set EPSRrun=" << workingDir_ << "\n"
                << ":loop\n"
                << "%EPSRbin%epsr.exe %EPSRrun% epsr " << baseFileName << "\n"
                << "if not exist %EPSRrun%killepsr ( goto loop ) else del %EPSRrun%killepsr\n";
#else
        stream << "export EPSRbin=" << epsrBinDir_ << "\n"
                << "export EPSRrun=" << workingDir_ << "\n"
                << "while :\n"
                << "do\n"
                << "  \"$EPSRbin\"'epsr' \"$EPSRrun\" epsr " << baseFileName << "\n"
                << "  if ([ -e \"$EPSRrun\"killepsr ])\n"
                << "  then break\n"
                << "  fi\n"
                << "done\n"
                << "rm -r \"$EPSRrun\"killepsr\n";
#endif
        batFile.close();
    }

    //run EPSR on loop
    QDir::setCurrent(workingDir_);

    QProcess processrunEPSRscript;
    processrunEPSRscript.setProcessChannelMode(QProcess::ForwardedChannels);
#ifdef _WIN32
    processrunEPSRscript.startDetached("run"+baseFileName+".bat");
#else
    processrunEPSRscript.startDetached("sh run"+baseFileName+".sh");
#endif

    //show EPSR is running
    ui.epsrRunningSign->setText("EPSR running");
    ui.epsrRunningSign->setEnabled(true);
    ui.stopAct->setEnabled(true);
    ui.runAct->setEnabled(false);
    ui.checkAct->setEnabled(false);

    //disable editing buttons while EPSR is running
    ui.newAct->setEnabled(false);
    ui.openAct->setEnabled(false);
    ui.saveAsAct->setEnabled(false);
    ui.saveCopyAct->setEnabled(false);
    ui.importAct->setEnabled(false);
    ui.exitAct->setEnabled(false);
    ui.deleteBoxAtoFileAct->setEnabled(false);
    ui.deleteEPSRinpFileAct->setEnabled(false);

    ui.updateMolFileButton->setDisabled(true);
    ui.updateAtoFileButton->setDisabled(true);
    ui.randomiseButton->setDisabled(true);
    ui.fmoleButton->setDisabled(true);
    ui.atoEPSRButton->setDisabled(true);
    ui.exchangeableAtomsButton->setDisabled(true);
    ui.makeWtsButton->setDisabled(true);

    ui.createMolFileButton->setEnabled(false);
    ui.molFileLoadButton->setEnabled(false);
    ui.createAtomButton->setEnabled(false);
    ui.createLatticeButton->setEnabled(false);
    ui.makeMolExtButton->setEnabled(false);
    ui.dockatoButton->setEnabled(false);
    ui.makelatticeatoButton->setEnabled(false);
    ui.removeMolFileButton->setEnabled(false);
    ui.addLJRowAboveButton->setEnabled(false);
    ui.addLJRowBelowButton->setEnabled(false);
    ui.deleteLJRowButton->setEnabled(false);
    ui.addDistRowAboveButton->setEnabled(false);
    ui.addDistRowBelowButton->setEnabled(false);
    ui.deleteDistRowButton->setEnabled(false);
    ui.addAngRowAboveButton->setEnabled(false);
    ui.addAngRowBelowButton->setEnabled(false);
    ui.deleteAngRowButton->setEnabled(false);
    ui.addDihRowAboveButton->setEnabled(false);
    ui.addDihRowBelowButton->setEnabled(false);
    ui.deleteDihRowButton->setEnabled(false);
    ui.deleteDihAllButton->setEnabled(false);
    ui.addRotRowAboveButton->setEnabled(false);
    ui.addRotRowBelowButton->setEnabled(false);
    ui.deleteRotRowButton->setEnabled(false);
    ui.deleteRotAllButton->setEnabled(false);
    ui.molChangeAtobutton->setEnabled(false);
    ui.molFmoleButton->setEnabled(false);
    ui.mixatoButton->setEnabled(false);
    ui.addatoButton->setEnabled(false);
    ui.loadBoxButton->setEnabled(false);
    ui.removeComponentButton->setEnabled(false);
    ui.dataFileBrowseButton->setEnabled(false);
    ui.removeDataFileButton->setEnabled(false);
    ui.setupEPSRButton->setEnabled(false);
    ui.updateInpPcofFilesButton->setEnabled(false);
    ui.setupOutButton->setEnabled(false);
    ui.applyOutputsButton->setEnabled(false);
    ui.applyCommandButton->setEnabled(false);
    ui.addOutputButton->setEnabled(false);
    ui.removeOutputButton->setEnabled(false);
    ui.dlputilsOutCheckBox->setEnabled(false);

    ui.messagesLineEdit->setText("EPSR is running in a terminal window");
    messageText_ += "\nEPSR is running in a terminal window.\n";
    messagesDialog.refreshMessages();

    //enable plotting as data files should now exist ************if this is clicked before files exist does program crash?
    ui.inpSettingsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui.dataFileSettingsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui.pcofSettingsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui.minDistanceTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui.plot1Button->setEnabled(true);
    ui.plot2Button->setEnabled(true);

    //add path for auto updating in case it is switched on
    epsrRunningTimerId_ = startTimer(20000);

    //also kill any other timers that might be still running if a setup was quit but not saved
    if (outputTimerId_ != -1) killTimer(outputTimerId_);
    outputTimerId_ = -1;
    if (molChangeatoFinishedTimerId_ != -1) killTimer(molChangeatoFinishedTimerId_);
    molChangeatoFinishedTimerId_ = -1;
    if (changeatoFinishedTimerId_ != -1) killTimer(changeatoFinishedTimerId_);
    changeatoFinishedTimerId_ = -1;
}

void MainWindow::stopEPSR()
{
    QFile file(workingDir_+"killepsr");
    if(!file.open(QFile::WriteOnly))
    {
        QMessageBox msgBox;
        msgBox.setText("Could not stop EPSR script");
        msgBox.exec();
        return;
    }
    file.close();
    ui.messagesLineEdit->setText("EPSR will stop at the end of this iteration");

    epsrFinished_.addPath(workingDir_+"killepsr");
}

void MainWindow::enableButtons()
{
    //turn off EPSR running sign
    ui.epsrRunningSign->setText("EPSR not running");
    ui.epsrRunningSign->setEnabled(false);
    ui.stopAct->setEnabled(false);
    ui.runAct->setEnabled(true);
    ui.checkAct->setEnabled(true);

    //re-enable editing buttons   
    ui.newAct->setEnabled(true);
    ui.openAct->setEnabled(true);
    ui.saveAsAct->setEnabled(true);
    ui.saveCopyAct->setEnabled(true);
    ui.importAct->setEnabled(true);
    ui.exitAct->setEnabled(true);
    ui.deleteBoxAtoFileAct->setEnabled(true);
    ui.deleteEPSRinpFileAct->setEnabled(true);

    ui.updateMolFileButton->setEnabled(true);
    ui.updateAtoFileButton->setEnabled(true);
    ui.randomiseButton->setEnabled(true);
    ui.fmoleButton->setEnabled(true);
    ui.atoEPSRButton->setEnabled(true);
    ui.exchangeableAtomsButton->setEnabled(true);
    ui.makeWtsButton->setEnabled(true);

    ui.createMolFileButton->setEnabled(true);
    ui.molFileLoadButton->setEnabled(true);
    ui.createAtomButton->setEnabled(true);
    ui.createLatticeButton->setEnabled(true);
    ui.makeMolExtButton->setEnabled(true);
    ui.dockatoButton->setEnabled(true);
    ui.makelatticeatoButton->setEnabled(true);
    ui.removeMolFileButton->setEnabled(true);
    ui.addLJRowAboveButton->setEnabled(true);
    ui.addLJRowBelowButton->setEnabled(true);
    ui.deleteLJRowButton->setEnabled(true);
    ui.addDistRowAboveButton->setEnabled(true);
    ui.addDistRowBelowButton->setEnabled(true);
    ui.deleteDistRowButton->setEnabled(true);
    ui.addAngRowAboveButton->setEnabled(true);
    ui.addAngRowBelowButton->setEnabled(true);
    ui.deleteAngRowButton->setEnabled(true);
    ui.addDihRowAboveButton->setEnabled(true);
    ui.addDihRowBelowButton->setEnabled(true);
    ui.deleteDihRowButton->setEnabled(true);
    ui.deleteDihAllButton->setEnabled(true);
    ui.addRotRowAboveButton->setEnabled(true);
    ui.addRotRowBelowButton->setEnabled(true);
    ui.deleteRotRowButton->setEnabled(true);
    ui.deleteRotAllButton->setEnabled(true);
    ui.molChangeAtobutton->setEnabled(true);
    ui.molFmoleButton->setEnabled(true);
    ui.mixatoButton->setEnabled(true);
    ui.addatoButton->setEnabled(true);
    ui.loadBoxButton->setEnabled(true);
    ui.removeComponentButton->setEnabled(true);
    ui.dataFileBrowseButton->setEnabled(true);
    ui.removeDataFileButton->setEnabled(true);
    ui.setupEPSRButton->setEnabled(true);
    ui.updateInpPcofFilesButton->setEnabled(true);
    ui.setupOutButton->setEnabled(true);
    ui.applyOutputsButton->setEnabled(true);
    ui.applyCommandButton->setEnabled(true);
    ui.addOutputButton->setEnabled(true);
    ui.removeOutputButton->setEnabled(true);
    ui.dlputilsOutCheckBox->setEnabled(true);

    ui.inpSettingsTable->setEditTriggers(QAbstractItemView::AllEditTriggers);
    ui.dataFileSettingsTable->setEditTriggers(QAbstractItemView::AllEditTriggers);
    ui.pcofSettingsTable->setEditTriggers(QAbstractItemView::AllEditTriggers);
    ui.minDistanceTable->setEditTriggers(QAbstractItemView::AllEditTriggers);

    ui.messagesLineEdit->setText("EPSR stopped");
    messageText_ += "\nEPSR stopped.\n";
    messagesDialog.refreshMessages();

    epsrFinished_.removePath(workingDir_+"killepsr");

    //remove path for auto update in case it is switched on
    if (epsrRunningTimerId_ != -1) killTimer(epsrRunningTimerId_);
    epsrRunningTimerId_ = -1;

    //if auto update not ticked, reload .inp and .pcof files and replot plots
    if (ui.autoUpdateCheckBox->isChecked() == false)
    {
        plot1();
        plot2();

        readEPSRinpFile();
        readEPSRpcofFile();
        updateInpFileTables();
        updatePcofFileTables();

        QFile file(baseFileName_+".EPSR.out");
        if (!file.open(QFile::ReadOnly | QFile::Text))
        {
            return;
        }

        QTextStream stream(&file);
        QString line;
        do
        {
            line = stream.readLine();
            messageText_ +=  line+"\n";
            messagesDialog.refreshMessages();
        } while (!line.isNull());
        file.close();
    }
}

void MainWindow::plot()
{
    PlotDialog plotDialog(this);

    plotDialog.setWindowTitle("EPSRgui plotting: "+projectName_);

    plotDialog.show();
    plotDialog.raise();
    plotDialog.activateWindow();
    plotDialog.exec();
}

void MainWindow::plotBox()
{
    PlotBoxDialog plotBoxDialog(this);

    plotBoxDialog.show();
    plotBoxDialog.raise();
    plotBoxDialog.activateWindow();
    plotBoxDialog.exec();
}

void MainWindow::plotEPSRshell()
{
    QDir::setCurrent(workingDir_);
    QString atoBaseFileName = atoFileName_.split(".",QString::SkipEmptyParts).at(0);

#ifdef _WIN32
    QFile batFile(workingDir_+"gnuplot"+atoBaseFileName+".bat");
#else
    QFile batFile(workingDir_+"gnuplot"+atoBaseFileName+".sh");
#endif
    if(!batFile.open(QFile::WriteOnly | QFile::Text))
    {
        QMessageBox msgBox;
        msgBox.setText("Could not open script file.");
        msgBox.exec();
        return;
    }

    QString gnuBinDir = epsrBinDir_+"gnuplot/binary";      //note this is without the last "/"
    gnuBinDir = QDir::toNativeSeparators(gnuBinDir);

    QTextStream stream(&batFile);
#ifdef _WIN32
    stream << "set EPSRbin=" << epsrBinDir_ << "\n"
            << "set EPSRrun=" << workingDir_ << "\n"
            << "set EPSRgnu=" << gnuBinDir << "\n"
            << "%EPSRbin%plot.exe %EPSRrun% plot\n";
#else
    stream << "export EPSRbin=" << epsrBinDir_ << "\n"
            << "export EPSRrun=" << workingDir_ << "\n"
            << "export EPSRgnu=" << gnuBinDir << "\n"
            << "  \"$EPSRbin\"'plot' \"$EPSRrun\" plot\n";
#endif
    batFile.close();

    QDir::setCurrent(workingDir_);

    QProcess processrunEPSRplot;
    processrunEPSRplot.setProcessChannelMode(QProcess::ForwardedChannels);
#ifdef _WIN32
    processrunEPSRplot.startDetached("gnuplot"+atoBaseFileName+".bat");
#else
    processrunEPSRplot.startDetached("sh gnuplot"+atoBaseFileName+".sh");
#endif

    ui.messagesLineEdit->setText("Started plot routine within EPSRshell");
}

void MainWindow::plotJmol()
{
    QString jmolFile = QFileDialog::getOpenFileName(this, "Choose .CUBE.txt file", workingDir_, tr(".CUBE.txt files (*.txt)"));
    if (!jmolFile.isEmpty())
    {
        QFileInfo fi(jmolFile);
        QString jmolFileName = fi.fileName();
        QString baseJmolFileName = jmolFileName.split(".", QString::KeepEmptyParts).at(0);
        QDir::setCurrent(workingDir_);

#ifdef _WIN32
        QFile batFile(workingDir_+"plot"+baseJmolFileName+".bat");
#else
        QFile batFile(workingDir_+"plot"+baseJmolFileName+".sh");
#endif
        if(!batFile.open(QFile::WriteOnly | QFile::Text))
        {
            QMessageBox msgBox;
            msgBox.setText("Could not open script file.");
            msgBox.exec();
            return;
        }

        QTextStream stream(&batFile);
#ifdef _WIN32
        stream << "set EPSRbin=" << epsrBinDir_ << "\n"
                << "set EPSRrun=" << workingDir_ << "\n"
                << "%EPSRbin%plot3djmol.exe %EPSRrun% plot3djmol " << jmolFileName << "\n";
#else
        stream << "export EPSRbin=" << epsrBinDir_ << "\n"
                << "export EPSRrun=" << workingDir_ << "\n"
                << "  \"$EPSRbin\"'plot3djmol' \"$EPSRrun\" plot3djmol " << jmolFileName << "\n";
#endif
        batFile.close();

        QDir::setCurrent(workingDir_);

        QProcess processrunEPSRplot;
        processrunEPSRplot.setProcessChannelMode(QProcess::ForwardedChannels);
#ifdef _WIN32
        processrunEPSRplot.startDetached("plot"+baseJmolFileName+".bat");
#else
        processrunEPSRplot.startDetached("sh plot"+baseJmolFileName+".sh");
#endif

        messageText_ += jmolFileName+" plotted in Jmol.\n";
        messagesDialog.refreshMessages();

        ui.messagesLineEdit->setText("Plotted .SHARM output in Jmol");
    }
}

void MainWindow::splot2d()
{
    //get filename
    QString plotFile = QFileDialog::getOpenFileName(this, "Choose .splot2d.txt file", workingDir_, tr(".splot2d.txt files (*.splot2d.txt)"));
    if (!plotFile.isEmpty())
    {
        QFileInfo fi(plotFile);
        QString plotFileName = fi.fileName();
        QString basePlotFileName = plotFileName.split(".", QString::KeepEmptyParts).at(0);
        QDir::setCurrent(workingDir_);

        //write script file
#ifdef _WIN32
        QFile batFile(workingDir_+"splot2d"+basePlotFileName+".bat");
#else
        QFile batFile(workingDir_+"splot2d"+basePlotFileName+".sh");
#endif
        if(!batFile.open(QFile::WriteOnly | QFile::Text))
        {
            QMessageBox msgBox;
            msgBox.setText("Could not open script file.");
            msgBox.exec();
            return;
        }

        QString gnuBinDir = epsrBinDir_+"gnuplot/binary";      //note this is without the last "/"
        gnuBinDir = QDir::toNativeSeparators(gnuBinDir);

        QTextStream stream(&batFile);
#ifdef _WIN32
        stream << "set EPSRbin=" << epsrBinDir_ << "\n"
                << "set EPSRrun=" << workingDir_ << "\n"
                << "set EPSRgnu=" << gnuBinDir << "\n"
                << "%EPSRbin%splot2d.exe %EPSRrun% splot2d " << basePlotFileName << "\n";
#else
        stream << "export EPSRbin=" << epsrBinDir_ << "\n"
                << "export EPSRrun=" << workingDir_ << "\n"
                << "export EPSRgnu=" << gnuBinDir << "\n"
                << "  \"$EPSRbin\"'splot2d' \"$EPSRrun\" splot2d " << basePlotFileName << "\n";
  #endif
        batFile.close();

        //run script file
        QProcess processrunEPSRplot;
        processrunEPSRplot.setProcessChannelMode(QProcess::ForwardedChannels);
#ifdef _WIN32
        processrunEPSRplot.startDetached("splot2d"+basePlotFileName+".bat");
#else
        processrunEPSRplot.startDetached("sh splot2d"+basePlotFileName+".sh");
#endif

        messageText_ += "\nfinished running "+plotFileName+"\n";
        messagesDialog.refreshMessages();
        ui.messagesLineEdit->setText("Finished running splot2d file");
    }
}

void MainWindow::plot2d()
{
    //get filename
    QString plotFile = QFileDialog::getOpenFileName(this, "Choose .plot2d.txt file", workingDir_, tr(".plot2d.txt files (*.plot2d.txt)"));
    if (!plotFile.isEmpty())
    {
        QFileInfo fi(plotFile);
        QString plotFileName = fi.fileName();
        QString basePlotFileName = plotFileName.split(".", QString::KeepEmptyParts).at(0);
        QDir::setCurrent(workingDir_);

        //make script file to run plot2d
#ifdef _WIN32
        QFile batFile(workingDir_+"plot2d"+basePlotFileName+".bat");
#else
        QFile batFile(workingDir_+"plot2d"+basePlotFileName+".sh");
#endif
        if(!batFile.open(QFile::WriteOnly | QFile::Text))
        {
            QMessageBox msgBox;
            msgBox.setText("Could not open script file.");
            msgBox.exec();
            return;
        }

        QTextStream stream(&batFile);
#ifdef _WIN32
        stream << "set EPSRbin=" << epsrBinDir_ << "\n"
                << "set EPSRrun=" << workingDir_ << "\n"
                << "%EPSRbin%plot2d.exe %EPSRrun% plot2d " << basePlotFileName << "\n"
                << "%EPSRrun%pgplot.gif\n";
#else
        stream << "export EPSRbin=" << epsrBinDir_ << "\n"
                << "export EPSRrun=" << workingDir_ << "\n"
                << "  \"$EPSRbin\"'plot2d' \"$EPSRrun\" plot2d " << basePlotFileName << "\n"
                << "\"$EPSRrun\"pgplot.gif\n";
#endif
        batFile.close();

        //run script file
        QProcess processPlot2d;
        processPlot2d.setProcessChannelMode(QProcess::ForwardedChannels);
#ifdef _WIN32
        processPlot2d.startDetached("plot2d"+basePlotFileName+".bat");
#else
        processPlot2d.startDetached("sh plot2d"+basePlotFileName+".sh");
#endif

        messageText_ += "\nfinished running "+basePlotFileName+" in plot2d\n";
        messagesDialog.refreshMessages();
        ui.messagesLineEdit->setText("Finished running plot2d file");
    }
}

void MainWindow::plot3d()
{
    //get filename
    QString plotFile = QFileDialog::getOpenFileName(this, "Choose .plot3d.txt file", workingDir_, tr(".plot3d.txt files (*.plot3d.txt)"));
    if (!plotFile.isEmpty())
    {
        QFileInfo fi(plotFile);
        QString plotFileName = fi.fileName();
        QString basePlotFileName = plotFileName.split(".", QString::KeepEmptyParts).at(0);
        QDir::setCurrent(workingDir_);

        //make script file to run plot3d
#ifdef _WIN32
        QFile batFile(workingDir_+"plot3d"+basePlotFileName+".bat");
#else
        QFile batFile(workingDir_+"plot3d"+basePlotFileName+".sh");
#endif
        if(!batFile.open(QFile::WriteOnly | QFile::Text))
        {
            QMessageBox msgBox;
            msgBox.setText("Could not open script file.");
            msgBox.exec();
            return;
        }

        QTextStream stream(&batFile);
#ifdef _WIN32
        stream << "set EPSRbin=" << epsrBinDir_ << "\n"
                << "set EPSRrun=" << workingDir_ << "\n"
                << "%EPSRbin%plot3d.exe %EPSRrun% plot3d " << basePlotFileName << "\n"
                << "%EPSRbin%pgplot.gif\n";
#else
        stream << "export EPSRbin=" << epsrBinDir_ << "\n"
                << "export EPSRrun=" << workingDir_ << "\n"
                << "  \"$EPSRbin\"'plot3d' \"$EPSRrun\" plot3d " << basePlotFileName << "\n"
                << "\"$EPSRrun\"pgplot.gif\n";
#endif
        batFile.close();

        //run script file
        QProcess processPlot3d;
        processPlot3d.setProcessChannelMode(QProcess::ForwardedChannels);
#ifdef _WIN32
        processPlot3d.startDetached("plot3d"+basePlotFileName+".bat");
#else
        processPlot3d.startDetached("sh plot3d"+basePlotFileName+".sh");
#endif

        messageText_ += "\nfinished running "+basePlotFileName+" in plot3d\n";
        messagesDialog.refreshMessages();
        ui.messagesLineEdit->setText("Finished running plot3d file");
    }
}

void MainWindow::settings()
{
    SettingsDialog settingsDialog(this);

    settingsDialog.setModal(true);
    settingsDialog.show();
    settingsDialog.raise();
    settingsDialog.activateWindow();

    setDialog = settingsDialog.exec();

    if (setDialog == SettingsDialog::Accepted)
    {
        epsrBinDir_.clear();
        epsrDir_.clear();
        visualiserExe_.clear();

        readSettings();

        ui.messagesLineEdit->setText("Updated settings");
    }
}

QString MainWindow::workingDir()
{
    return workingDir_;
}

QString MainWindow::atoFileName()
{
    return atoFileName_;
}

QString MainWindow::epsrInpFileName()
{
    return epsrInpFileName_;
}

QString MainWindow::epsrDir()
{
    return epsrDir_;
}

QString MainWindow::epsrBinDir()
{
    return epsrBinDir_;
}

QStringList MainWindow::atomTypes()
{
    return atoAtomTypes;
}

QVector<int> MainWindow::numberOfEachAtomType()
{
    return numberAtomTypes;
}

QStringList MainWindow::listAtoFiles()
{
    atoFileList.clear();
    for (int i = 0; i < ui.atoFileTable->rowCount(); i++)
    {
        atoFileList.append(ui.atoFileTable->item(i,0)->text());
    }
    return atoFileList;
}

QStringList MainWindow::listWtsFiles()
{
    return wtsFileList;
}

QString MainWindow::wtsBaseFileName()
{
    return wtsBaseFileName_;
}

QString MainWindow::outputFileName()
{
    return outputFileName_;
}

QString MainWindow::outputSetupFileType()
{
    return outputSetupFileType_;
}

QString MainWindow::outputFileExt()
{
    return outputFileExt_;
}

int MainWindow::showOutputType()
{
    return outputType_;
}

QString MainWindow::plotFileName()
{
    return plotFileName_;
}

QString MainWindow::plotSetupFileType()
{
    return plotSetupFileType_;
}

QString MainWindow::plotFileExt()
{
    return plotFileExt_;
}

QString MainWindow::coeffFileName()
{
    return coeffFileName_;
}

QByteArray MainWindow::messageText()
{
    return messageText_;
}

void MainWindow::deleteEPSRinpFile()
{
    QFile file(workingDir_+epsrInpFileName_);
    if(file.exists() == true)
    {
        QMessageBox::StandardButton msgBox;
        msgBox  = QMessageBox::question(this, "Warning", "This will delete the existing EPSR.inp file.\nProceed?",
                                        QMessageBox::Ok|QMessageBox::Cancel);
        if (msgBox == QMessageBox::Cancel)
        {
            return;
        }
        else
        {
            ui.messagesLineEdit->setText("Please wait: deleting EPSR inp file");
            ui.tabWidget->setDisabled(true);

            QDir dir(workingDir_);
#ifdef _WIN32
            dir.setNameFilters(QStringList() << "*.EPSR.*" << "run*.bat" << "*.pcof");
#else
            dir.setNameFilters(QStringList() << "*.EPSR.*" << "run*.sh" << "*.pcof");
#endif
            dir.setFilter(QDir::Files);
            foreach (QString EPSRfile, dir.entryList())
            {
                dir.remove(EPSRfile);
            }
            epsrInpFileName_.clear();
            ui.epsrInpFileName->clear();
            ui.inpSettingsTable->clearContents();
            ui.inpSettingsTable->setRowCount(0);
            ui.dataFileSettingsTable->clearContents();
            ui.dataFileSettingsTable->setRowCount(0);
            ui.pcofSettingsTable->clearContents();
            ui.pcofSettingsTable->setRowCount(0);
            ui.minDistanceTable->clearContents();
            ui.minDistanceTable->setRowCount(0);
            epsrInpFileName_.clear();
            ui.runOutEPSRList->clear();
            ui.dlputilsOutCheckBox->setChecked(false);

            //clear plots
            ui.plot1->clearGraphs();
            ui.plot1->clearItems();
            ui.plot1->clearPlottables();
            ui.plot1->replot();
            ui.plot2->clearGraphs();
            ui.plot2->clearItems();
            ui.plot2->clearPlottables();
            ui.plot2->replot();

            //enable/disable buttons
            ui.runAct->setEnabled(false);
            ui.stopAct->setEnabled(false);
            ui.plotAct->setEnabled(false);
            ui.plotEPSRshellAct->setEnabled(false);
            ui.plot3dAct->setEnabled(false);
            ui.deleteEPSRinpFileAct->setEnabled(false);
            ui.plot1Button->setEnabled(false);
            ui.plot2Button->setEnabled(false);
            ui.dataFileBrowseButton->setEnabled(true);
            ui.removeDataFileButton->setEnabled(true);
            ui.removeComponentButton->setEnabled(true);
            ui.updateInpPcofFilesButton->setEnabled(false);
            ui.reloadEPSRinpButton->setEnabled(false);
            ui.setupOutButton->setEnabled(false);
            ui.setupPlotButton->setEnabled(false);
            ui.applyOutputsButton->setEnabled(false);
            ui.applyCommandButton->setEnabled(false);
            ui.dlputilsOutCheckBox->setEnabled(false);

            messageText_ += "EPSR.inp and associated files deleted\n";
            messagesDialog.refreshMessages();
            ui.messagesLineEdit->setText("EPSR.inp file deleted");

            save();
            ui.tabWidget->setDisabled(false);
        }
    }
    return;
}

void MainWindow::deleteBoxAtoFile()
{
    if (!epsrInpFileName_.isEmpty())
    {
        deleteEPSRinpFile();
    }

    QFile file(workingDir_+atoFileName_);
    if(file.exists() == true)
    {
        QMessageBox::StandardButton msgBox;
        msgBox  = QMessageBox::question(this, "Warning", "This will delete the existing box .ato file and any wts files.\nProceed?", QMessageBox::Ok|QMessageBox::Cancel);
        if (msgBox == QMessageBox::Cancel)
        {
            return;
        }
        else
        {
            ui.messagesLineEdit->setText("Please wait: deleting box and associated files");
            ui.tabWidget->setDisabled(true);

            //remove box ato file, clear name and re-initialise ato tab
            file.remove();
            atoFileName_.clear();
            ui.boxAtoLabel->clear();
            ui.boxAtoCharge->clear();
            ui.boxAtoAxisA->clear();
            ui.boxAtoAxisB->clear();
            ui.boxAtoAxisG->clear();
            ui.boxAtoLengthA->clear();
            ui.boxAtoLengthB->clear();
            ui.boxAtoLengthC->clear();
            ui.boxAtoMols->clear();
            ui.boxAtoVol->clear();
            ui.boxAtoAtoms->clear();
            ui.atoTetherTable->clearContents();
            ui.atoTetherTable->setRowCount(0);
            ui.atoTetherTolLineEdit->setText("0.00000E+00");
            ui.temperatureLineEdit->clear();
            ui.vibtempLineEdit->clear();
            ui.angtempLineEdit->clear();
            ui.dihtempLineEdit->clear();
            ui.intraTransSSLineEdit->clear();
            ui.grpRotSSLineEdit->clear();
            ui.molRotSSLineEdit->clear();
            ui.molTransSSLineEdit->clear();
            ui.ecoreLineEdit->clear();
            ui.dcoreLineEdit->clear();
            ui.numberDensityLineEdit->clear();

            //remove wts files and clear data and wts tables
            dataFileList.clear();
            wtsFileList.clear();
            normalisationList.clear();
            ui.normalisationComboBox->setCurrentIndex(0);
            ui.atomWtsTable->clearContents();
            ui.atomWtsTable->setRowCount(0);
            ui.dataFileTable->setRowCount(0);
            ui.exchangeableAtomsButton->setEnabled(false);
            ui.makeWtsButton->setEnabled(false);
            ui.setupEPSRButton->setEnabled(false);
            ui.dataFileLineEdit->clear();
            QDir dir(workingDir_);
            dir.setNameFilters(QStringList() << "*.NWTS.dat" << "*.XWTS.dat" << "*.wts");
            dir.setFilter(QDir::Files);
            foreach(QString dirFile, dir.entryList())
            {
                dir.remove(dirFile);
            }

            //enable/disable buttons
            ui.deleteBoxAtoFileAct->setEnabled(false);
            ui.randomiseButton->setEnabled(false);
            ui.boxCompositionButton->setEnabled(false);
            ui.updateAtoFileButton->setEnabled(false);
            ui.fmoleButton->setEnabled(false);
            ui.atoEPSRButton->setEnabled(false);
            ui.reloadBoxButton->setEnabled(false);
            ui.removeComponentButton->setEnabled(false);
            ui.dataFileBrowseButton->setEnabled(false);
            ui.removeDataFileButton->setEnabled(false);
            ui.exchangeableAtomsButton->setEnabled(false);
            ui.makeWtsButton->setEnabled(false);
            ui.setupEPSRButton->setEnabled(false);
            ui.updateInpPcofFilesButton->setEnabled(false);
            ui.reloadEPSRinpButton->setEnabled(false);
            ui.setupOutButton->setEnabled(false);
            ui.setupPlotButton->setEnabled(false);
            ui.applyOutputsButton->setEnabled(false);
            ui.applyCommandButton->setEnabled(false);
            ui.dlputilsOutCheckBox->setEnabled(false);

            //clear plots
            ui.plot1->clearGraphs();
            ui.plot1->clearItems();
            ui.plot1->replot();
            ui.plot2->clearGraphs();
            ui.plot2->clearItems();
            ui.plot2->replot();

            save();

            ui.tabWidget->setDisabled(false);
            messageText_ += "box .ato file deleted\n";
            messagesDialog.refreshMessages();
            ui.messagesLineEdit->setText("box .ato file deleted");
        }
    }
    return;
}

void MainWindow::showMessages()
{
    messagesDialog.show();
    messagesDialog.raise();
    messagesDialog.activateWindow();
    messagesDialog.exec();
}

void MainWindow::showNotes()
{
    NotesDialog notesDialog(this);

    notesDialog.setModal(true);
    notesDialog.show();
    notesDialog.raise();
    notesDialog.activateWindow();
    notesDialog.exec();
}

void MainWindow::outputfromEPSRprocessReady()
{
    messageText_ += processEPSR_.readAllStandardOutput();
    messagesDialog.refreshMessages();
}

void MainWindow::openEPSRmanual()
{
    if (epsrBinDir_.isEmpty())
    {
        QMessageBox msgBox;
        msgBox.setText("The EPSR binaries directory needs to be defined.");
        msgBox.exec();
        return;
    }

    QStringList filters;
    filters << "*.pdf";
    QString docDirstr = epsrBinDir_.split("bin", QString::SkipEmptyParts).at(0);
    docDirstr = docDirstr+"doc/";
    QDir docDir = QDir::toNativeSeparators(docDirstr);
    docDir.setSorting(QDir::Time);
    QStringList pdfFiles = docDir.entryList(filters, QDir::Files);
    if (pdfFiles.isEmpty())
    {
        QMessageBox msgBox;
        msgBox.setText("Could not find any .pdf files");
        msgBox.exec();
        return;
    }

    QString filename;
    for (int i = 0 ; i < pdfFiles.count(); i++)
    {
        filename = pdfFiles.at(i);
        if (filename.contains("EPSRshell"))
        {
            break;
        }
    }

    QString manual = docDirstr+filename;
    QDesktopServices::openUrl(QUrl("file:///"+manual, QUrl::TolerantMode));
}

void MainWindow::openEPSRguiManual()
{
#ifdef Q_OS_MAC
    QString guimanual = exeDir_.path().split("MacOS", QString::KeepEmptyParts).at(0)+"SharedSupport/EPSRgui Manual.pdf";
    QFile guimanualfile(guimanual);
    if (!guimanualfile.exists())
    {
        guimanual = exeDir_.path()+"/resources/EPSRgui Manual.pdf";
    }
#elif _WIN32
    QString guimanual = exeDir_.path().split("bin", QString::KeepEmptyParts).at(0)+"resources/EPSRgui Manual.pdf";

#else
    QString guimanual = exeDir_.path()+"/resources/EPSRgui Manual.pdf";
    guimanual = QDir::toNativeSeparators(guimanual);
#endif
    QDesktopServices::openUrl(QUrl("file:///"+guimanual, QUrl::TolerantMode));
}

void MainWindow::autoUpdate()
{
    if (ui.autoUpdateCheckBox->isChecked() == false)
    {
        return;
    }
    plot1();
    plot2();

    readEPSRinpFile();
    readEPSRpcofFile();
    updateInpFileTables();
    updatePcofFileTables();

    QFile file(baseFileName_+".EPSR.out");
    if (!file.open(QFile::ReadOnly | QFile::Text))
    {
        return;
    }

    QTextStream stream(&file);
    QString line;
    do
    {
        line = stream.readLine();
        messageText_ +=  line+"\n";
        messagesDialog.refreshMessages();
    } while (!line.isNull());
    file.close();
}

void MainWindow::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == outputTimerId_)
    {
        QFile outfile(workingDir_+outputFileName_+outputFileExt_);
        QFile plotFile(workingDir_+plotFileName_+plotFileExt_);
        if (outfile.exists() || plotFile.exists())
        {
            showAvailableFiles();
            if (outputTimerId_ != -1) killTimer(outputTimerId_);
            outputTimerId_ = -1;
        }
    }
    else
    if (event->timerId() == epsrRunningTimerId_)
    {
        QFileInfo fi(workingDir_+epsrInpFileName_);
        int secs = fi.lastModified().secsTo(QDateTime::currentDateTime());
        if (secs < 21)
        {
            autoUpdate();
        }
    }
    else
    if (event->timerId() == fmoleFinishedTimerId_)
    {
        QFileInfo fi(workingDir_+atoFileName_);
        if (fi.lastModified() > atoLastMod_)
        {
            //reenable buttons
            ui.fmoleButton->setEnabled(true);
            ui.fmoleLineEdit->setEnabled(true);
            ui.updateAtoFileButton->setEnabled(true);
            ui.mixatoButton->setEnabled(true);
            ui.addatoButton->setEnabled(true);
            ui.loadBoxButton->setEnabled(true);
            ui.randomiseButton->setEnabled(true);
            ui.atoEPSRButton->setEnabled(true);
            ui.reloadBoxButton->setEnabled(true);
            ui.removeComponentButton->setEnabled(true);
            ui.updateMolFileButton->setEnabled(true);
            ui.runMenu->setEnabled(true);

            messageText_ += "\nfmole finished running on box.ato file\n";
            messagesDialog.refreshMessages();
            ui.messagesLineEdit->setText("Finished running fmole");

            if (fmoleFinishedTimerId_ != -1) killTimer(fmoleFinishedTimerId_);
            fmoleFinishedTimerId_ = -1;
        }
    }
    else
    if (event->timerId() == molChangeatoFinishedTimerId_)
    {
        QString atoFileName = molFileName_.split(".", QString::SkipEmptyParts).at(0)+".ato";
        QFileInfo fi(workingDir_+atoFileName);
        if (fi.lastModified() > atoLastMod_)
        {
            setSelectedMolFile();

            messageText_ += "\nComponent .ato file updated\n";
            messagesDialog.refreshMessages();
            ui.messagesLineEdit->setText("Component .ato file updated");

            if (molChangeatoFinishedTimerId_ != -1) killTimer(molChangeatoFinishedTimerId_);
            molChangeatoFinishedTimerId_ = -1;
        }
    }
    else
    if (event->timerId() == changeatoFinishedTimerId_)
    {
        QFileInfo fi(workingDir_+atoFileName_);
        if (fi.lastModified() > atoLastMod_)
        {
            messageText_ += "\nBox .ato file updated\n";
            messagesDialog.refreshMessages();
            ui.messagesLineEdit->setText("Box .ato file updated");

            if (changeatoFinishedTimerId_ != -1) killTimer(changeatoFinishedTimerId_);
            changeatoFinishedTimerId_ = -1;
        }
    }
}
