#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "molfiles.h"
#include "ui_epsrproject.h"
#include "wtscomponent.h"
#include "datafilesettings.h"
#include "array.h"
#include "messagesdialog.h"
#include "notesdialog.h"

class CreateNewDialog;
class PlotDialog;
class MolOptionsDialog;
class SettingsDialog;
class MakeAtomDialog;
class MakeLatticeDialog;
class BoxCompositionDialog;
class AddAtoDialog;
class ImportDialog;
class PlotBoxDialog;
class SetupOutputDialog;
class NotesDialog;
class RemoveComponentDialog;
class MakeMolLattDialog;
class ExchangeableAtomsDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QMainWindow *parent = 0);
    Ui::MainWindow ui;
    int newDialog;
    int newImportDialog;
    int setDialog;

protected:
    void closeEvent(QCloseEvent *event);
    void timerEvent(QTimerEvent* event);

private slots:
    bool save();
    bool saveAs();
    bool saveCopy();
    void import();
    void runEPSRonce();
    void runEPSR();
    void plot();
    void plotBox();
    void plotEPSRshell();
    void plotJmol();
    void splot2d();
    void plot2d();
    void plot3d();
    void settings();
    void stopEPSR();
    void deleteEPSRinpFile();
    void deleteBoxAtoFile();
    void showMessages();
    void showNotes();
    void openEPSRmanual();
    void openEPSRguiManual();
    void outputfromEPSRprocessReady();
    void autoUpdate();

private:
    void createActions();
    void reset();
    void readSettings();

    MolOptionsDialog *molOptionsDialog;
    MakeAtomDialog *makeAtomDialog;
    MakeLatticeDialog *makeLatticeDialog;
    MessagesDialog messagesDialog;

    QDir exeDir_;
    QDir currentDir;
    QString projectName_; //folder name for simulation but doesn't include path or a "/" after it
    QString workingDir_; //includes projectName and has a "/" after it
    QString epsrDir_; //this is the preferred dir that projects are saved in and has a "/" after it
    QString epsrBinDir_; //has a "/" after it
    QString baseFileName_; //use this for the name of the box and the EPSR inp files with the path and without any extensions
    QString molFileName_; //just filename and extension for selected file in molFileList (.mol or .ato), no path
    QString visualiserExe_; //full path and executable
    QString atoFileName_; //just filename and extension for box, no path
    QString dataFileName_; //filename and extension for selected file in dataFileTable, no path
    QString wtsBaseFileName_; //only base file name, not including extension or path for selected file in dataFileTable
    QString dataFileExt_; //data file extension
    QString epsrInpFileName_; //filename and extension for EPSR.inp file
    QFileSystemWatcher epsrFinished_;
    QFileSystemWatcher jmolFile_;
    int inpEntries_;
    QByteArray messageText_;
    QProcess processEPSR_;
    int outputTimerId_;
    int epsrRunningTimerId_;
    int fmoleFinishedTimerId_;
    QDateTime atoLastMod_;
    int molChangeatoFinishedTimerId_;
    int changeatoFinishedTimerId_;

    int endscript;

public:
    QString workingDir();
    QString atoFileName();
    QString epsrInpFileName();
    QString epsrDir();
    QString epsrBinDir();
    QStringList atomTypes();
    QVector<int> numberOfEachAtomType();
    QStringList listAtoFiles();
    QStringList listWtsFiles();
    QString wtsBaseFileName();
    QString outputFileName();
    QString outputSetupFileType();
    QString outputFileExt();
    int showOutputType();
    QString plotFileName();
    QString plotSetupFileType();
    QString plotFileExt();
    QString coeffFileName();
    QByteArray messageText();
    QStringList exchangeableAtomsList();

public slots:
    void createNew();
    void open();
    void showPointToolTip1(QMouseEvent *event);
    void showPointToolTip2(QMouseEvent *event);
    void plotZoom1(QWheelEvent *event);
    void plotZoom2(QWheelEvent *event);
    void enableButtons();
    void makeMolFile();

    // Mol Files Tab
private slots:
    void on_createMolFileButton_clicked(bool checked);
    void on_molFileLoadButton_clicked(bool checked);
    void on_createAtomButton_clicked(bool checked);
    void on_createLatticeButton_clicked(bool checked);
    void on_removeMolFileButton_clicked(bool checked);
    void on_updateMolFileButton_clicked(bool checked);
    void on_molFileList_itemSelectionChanged();
    void on_viewMolFileButton_clicked(bool checked);
    void setSelectedMolFile();
    void on_addLJRowAboveButton_clicked(bool checked);
    void on_addLJRowBelowButton_clicked (bool checked);
    void on_deleteLJRowButton_clicked (bool checked);
    void on_addDistRowAboveButton_clicked(bool checked);
    void on_addDistRowBelowButton_clicked (bool checked);
    void on_deleteDistRowButton_clicked (bool checked);
    void on_addAngRowAboveButton_clicked(bool checked);
    void on_addAngRowBelowButton_clicked (bool checked);
    void on_deleteAngRowButton_clicked (bool checked);
    void on_addDihRowAboveButton_clicked(bool checked);
    void on_addDihRowBelowButton_clicked (bool checked);
    void on_deleteDihRowButton_clicked (bool checked);
    void on_deleteDihAllButton_clicked (bool checked);
    void on_addRotRowAboveButton_clicked(bool checked);
    void on_addRotRowBelowButton_clicked (bool checked);
    void on_deleteRotRowButton_clicked (bool checked);
    void on_deleteRotAllButton_clicked (bool checked);
    void on_molChangeAtobutton_clicked(bool checked);
    void on_makeMolExtButton_clicked(bool checked);
    void on_molFmoleButton_clicked(bool checked);
    void on_dockatoButton_clicked(bool checked);

public:
    QList<IntraMolecular> bonds;
    int mopacOption;
    int molCharge;
    int molDialog;
    int atomDialog;
    int latticeDialog;
    int nMolFiles;

private:
    bool readMolFile();
    bool updateMolFile();
    bool readAtoFile();
    bool updateAtoFile();

    // Box Ato File Tab
private slots:
    void on_mixatoButton_clicked(bool checked);
    void on_addatoButton_clicked(bool checked);
    void on_loadBoxButton_clicked (bool checked);
    void on_makelatticeatoButton_clicked(bool checked);
    void on_randomiseButton_clicked(bool checked);
    void on_updateAtoFileButton_clicked(bool checked);
    void on_fmoleButton_clicked(bool checked);
    void on_atoEPSRButton_clicked(bool checked);
    void on_boxCompositionButton_clicked(bool checked);
    void on_SSbutton_clicked(bool checked);
    void on_tetherButton_clicked(bool checked);
    void on_reloadBoxButton_clicked(bool checked);
    void on_removeComponentButton_clicked(bool checked);

private:
    bool readAtoFileBoxDetails();
    bool checkBoxCharge();

public:
    int atoaddDialog;
    int mollattDialog;
    int componentremoveDialog;
    QStringList atoComponentList; //lists components in order found in ato file (theoretically the same as molFileList and atoFileTable, but used when don't know what is in the box
    QStringList atoAtomTypes; //atom Types as listed from start of box .ato file
    QVector<int> numberAtomTypes; //number of instances of each item in atoAtomTypes in box .ato file
    QVector<int> numberComponentAtomLabels; //number of instances of each item in ljAtoms in component .mol or .ato file
    int nPartials;
    Array2D<int> ij;
    QStringList atoFileList;
    int atoHeaderLines; //number header lines in box .ato file
    QStringList firstAtomList; //list of first atom Types for each component in order of ui.molFileList
    QVector<int> nInBox; //number of instances of each component in box (in order of ui.molFileList)
    QList<int> nLinesPerComponentList; //number of lines per each component (not group of components) in order of atoFileList
    QList<int> firstInstance; //first instance of component in numerical listing in box ato file per component (atoFileList order)
    QList<int> lastInstance; //last instance of component in numerical listing in box ato file per component (atoFileList order)

    // Wts Files Tab
private slots:
    void on_dataFileBrowseButton_clicked(bool checked);
    void on_dataFileTable_itemSelectionChanged();
    void setSelectedDataFile();
    void on_removeDataFileButton_clicked(bool checked);
    void on_makeWtsButton_clicked(bool checked);
    void on_exchangeableAtomsButton_clicked(bool checked);

private:
    bool makeNwtsSetup();
    bool readNwtsSetup();
    bool makeXwtsSetup();
    bool readXwtsSetup();
    void makeNwts();
    void makeXwts();
    void refreshDataFileTable();

public:
    QList<WtsComponent> wtscomponents;
    QStringList dataFileList;
    QStringList wtsFileList;
    QStringList normalisationList;
    int addExchangeableAtoms;

    // Setup EPSR Inp File Tab
private slots:
    void on_setupEPSRButton_clicked(bool checked);
    void on_updateInpPcofFilesButton_clicked(bool checked);
    void on_reloadEPSRinpButton_clicked(bool checked);

private:
    bool readEPSRinpFile();
    void updateInpFileTables();
    void updateInpFile();
    bool readEPSRpcofFile();
    void updatePcofFile();
    void updatePcofFileTables();

public:
    QList<DataFileSettings> datafilesettings;
    QStringList inpDescriptions;
    QStringList inpKeywords;
    QStringList inpValues;
    QStringList pcofDescriptions;
    QStringList pcofKeywords;
    QStringList pcofValues;
    QStringList atomPairs;
    QStringList minDistances;

    //epsr outputs
private:
    void setupOutput();
    void setupPlot();
    QString outputFileName_;
    QString outputSetupFileType_;
    QString outputFileExt_;
    QString outputRunFileType_;
    QString plotSetupFileType_;
    QString plotFileName_;
    QString plotFileExt_;
    int outputType_;
    QString coeffFileName_;
    void showAvailableFiles();
    void getOutputsRunning();
    void addOutputsToScript();

private slots:
    void on_setupOutButton_clicked(bool checked);
    void on_setupPlotButton_clicked(bool checked);
    void getOutputType();
    void on_addOutputButton_clicked(bool checked);
    void on_removeOutputButton_clicked(bool checked);
    void on_applyOutputsButton_clicked(bool checked);
    void on_applyCommandButton_clicked(bool checked);

    //main window embedded plots
private:
    void plot1();
    void plot2();
    bool getplottype1();
    bool getplottype2();
    bool fqplot1();
    bool frplot1();
    bool Eplot1();
    bool Rplot1();
    bool fqplot2();
    bool frplot2();
    bool Eplot2();
    bool Rplot2();
    bool Pplot1();
    bool Pplot2();
    bool Ereqplot1();
    bool Ereqplot2();

private slots:
    void on_plot1Button_clicked();
    void on_plot2Button_clicked();

};

#endif
