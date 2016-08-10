#include "plotdialog.h"
#include "array.h"
#include "qcustomplot.h"
#include "epsrproject.h"

#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QString>
#include <QLabel>
#include <QVector>
#include <QGuiApplication>

PlotDialog::PlotDialog(MainWindow *parent) : QDialog(parent)
{
    ui.setupUi(this);
    mainWindow_ = parent;

    workingDir_ = mainWindow_->workingDir();
    atoFileName_ = mainWindow_->atoFileName();
    baseFileName_ = workingDir_+atoFileName_.split(".").at(0);

    ui.standardPlotList->setCurrentRow(0);
    connect(ui.closeButton, SIGNAL(clicked()), this, SLOT(reject()));
    readatofile();
    getnDataCol();

    //get list of outputs that are in folder (this is everything in the folder, not what is currently in the epsr script file)
    populateOutputsList();

    //hide optionsGroupBox
    ui.optionsGroupBox->setVisible(false);

    setWindowFlags(windowFlags() | Qt::WindowMinimizeButtonHint);
    setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint);

    //MouseTracking;
    connect(ui.customPlot, SIGNAL(mouseMove(QMouseEvent*)), this, SLOT(showPointToolTip(QMouseEvent*)));
    connect(ui.customPlot, SIGNAL(mouseWheel(QWheelEvent*)), this, SLOT(plotZoom(QWheelEvent*)));
}

void PlotDialog::populateOutputsList()
{
    QDir::setCurrent(workingDir_);
    QDir dir;
    QStringList outputFilesFilters;
    outputFilesFilters << "*.CHAINS.n01" << "*.CLUSTERS.n01" << "*.COORD.n01" << ".FLUCTUATIONS.n01"
                       << "*.RINGS.n01"<< "*.TOR.c01" << "*.TRI.c01" << "*.VOIDS.n01";
    QStringList outputFilesList = dir.entryList(outputFilesFilters, QDir::Files);
    if (outputFilesList.isEmpty())
    {
        return;
    }

    for (int i = 0; i < outputFilesList.count(); i++)
    {
        ui.outputPlotList->addItem(outputFilesList.at(i));
    }
}

void PlotDialog::on_standardPlotList_itemClicked(QListWidgetItem *item)
{
    ui.outputPlotList->setCurrentRow(-1);
    ui.outputAtomSetList->clear();
}

void PlotDialog::on_outputPlotList_itemClicked(QListWidgetItem *item)
{
    ui.standardPlotList->setCurrentRow(-1); //deselects standardPlotList
    ui.outputAtomSetList->clear();
    //list atom sets in ui.outputAtomSetList  -check which file type is selected and then read 1st line of file

    QString FileName = workingDir_+ui.outputPlotList->currentItem()->text();
    QFile file(FileName);

    if (ui.outputPlotList->currentItem()->text().contains("COORD"))
    {
        if(!file.open(QFile::ReadOnly | QFile::Text))
        {
            QMessageBox msgBox;
            msgBox.setText("Could not open "+FileName);
            msgBox.exec();
            return;
        }
        QTextStream stream(&file);
        QString line;
        QStringList dataLine;
        dataLine.clear();

        //read first line to get number of columns (and therefore atom sets)
        line = stream.readLine();
        dataLine = line.split(" ", QString::SkipEmptyParts);
        if (dataLine.count() == 0) return;
        int numberSets = (dataLine.count()-2);
        for (int i = 2; i < numberSets+2; i++)
        {
            ui.outputAtomSetList->addItem(dataLine.at(i).split("(", QString::SkipEmptyParts).at(0));
        }
        ui.outputAtomSetList->setCurrentRow(0);
    }
    else
    if (ui.outputPlotList->currentItem()->text().contains("CHAINS"))
    {
        if(!file.open(QFile::ReadOnly | QFile::Text))
        {
            QMessageBox msgBox;
            msgBox.setText("Could not open "+FileName);
            msgBox.exec();
            return;
        }
        QTextStream stream(&file);
        QString line;
        QStringList dataLine;
        dataLine.clear();

        //read first line to get number of columns (and therefore atom sets)
        line = stream.readLine();
        dataLine = line.split(" ", QString::SkipEmptyParts);
        if (dataLine.count() == 0) return;
        int numberSets = (dataLine.count()-2)/2; //CHAINS always lists each atom set twice so just use one instance of each
        for (int i = 2; i < numberSets*2+2; i+=2)
        {
            ui.outputAtomSetList->addItem(dataLine.at(i));
        }
        ui.outputAtomSetList->setCurrentRow(0);
    }
    else
    {
        if(!file.open(QFile::ReadOnly | QFile::Text))
        {
            QMessageBox msgBox;
            msgBox.setText("Could not open "+FileName);
            msgBox.exec();
            return;
        }
        QTextStream stream(&file);
        QString line;
        QStringList dataLine;
        dataLine.clear();

        //read first line to get number of columns (and therefore atom sets)
        line = stream.readLine();
        dataLine = line.split(" ", QString::SkipEmptyParts);
        if (dataLine.count() == 0) return;
        int numberSets = (dataLine.count()-2);
        for (int i = 2; i < numberSets+2; i++)
        {
            ui.outputAtomSetList->addItem(dataLine.at(i));
        }
        ui.outputAtomSetList->setCurrentRow(0);
    }
}

void PlotDialog::on_plotButton_clicked(bool checked)
{
    ui.customPlot->clearGraphs();
    ui.customPlot->clearItems();
    ui.customPlot->clearPlottables();
    getplottype();
}

void PlotDialog::showPointToolTip(QMouseEvent *event)
{

    double x = this->ui.customPlot->xAxis->pixelToCoord(event->pos().x());
    double y = this->ui.customPlot->yAxis->pixelToCoord(event->pos().y());

//    setToolTip(QString("%1 , %2").arg(x).arg(y));

    QString xstr;
    xstr.sprintf(" %5g", x);
    QString ystr;
    ystr.sprintf(" %5g", y);
    QString value = xstr+","+ystr;
    ui.mousecoord->setText(value);
}

void PlotDialog::plotZoom(QWheelEvent* event)
{
    if (event->modifiers() == Qt::ShiftModifier)
    {
        ui.customPlot->axisRect()->setRangeZoomAxes(ui.customPlot->xAxis,ui.customPlot->xAxis);
    }
    else if (event->modifiers() == Qt::ControlModifier)
    {
        ui.customPlot->axisRect()->setRangeZoomAxes(ui.customPlot->yAxis,ui.customPlot->yAxis);
    }
    else
    {
        ui.customPlot->axisRect()->setRangeZoomAxes(ui.customPlot->xAxis,ui.customPlot->yAxis);
    }
}

void PlotDialog::on_closeButton_clicked(bool checked)
{
    ui.customPlot->clearGraphs();
}

//read ato file and construct atom pair array for plot
bool PlotDialog::readatofile()
{
    QFile file(workingDir_+atoFileName_);
    if(!file.open(QFile::ReadOnly | QFile::Text))
    {
        QMessageBox msgBox;
        msgBox.setText("Could not open .ato file. Check .ato file exists in folder and is accessible in order to plot g(r)s");
        msgBox.exec();
        return 0;
    }
    QTextStream stream(&file);
    QString line;
    QRegExp atomlabelrx(" ([A-Z][A-Za-z0-9 ]{2}) ([A-Za-z ]{1,2})   ([0-1]{1})");
    if (!atomlabelrx.isValid()) printf("REGEXP is not valid.\n");
    atomLabels.clear();

    do {
        line = stream.readLine();
        if (atomlabelrx.exactMatch(line))
        {
            atomLabels << atomlabelrx.cap(1);
        }

    } while (!line.isNull());

    //close ato file
    file.close();

    //number of entries in atomLabels list
    const int N_atomLabels = atomLabels.count();

    //show atomLabels in dialog
    ui.listWidget->clear();
    ui.listWidget_2->clear();
    ui.listWidget->addItems(atomLabels);
    ui.listWidget->setCurrentRow(0);
    ui.listWidget_2->addItems(atomLabels);
    ui.listWidget_2->setCurrentRow(0);

    // Declare and initialise matrix for column to find atom pair partial in...
    ij.initialise(N_atomLabels,N_atomLabels);

    // Populate it: first atompair_index = 0
    int count = 0;
    for (int i = 0; i < N_atomLabels; ++i)
    {
        for (int j = i; j < N_atomLabels; ++j)
        {
            ij.ref(i,j) = count;
            ij.ref(j,i) = count;
            ++count;
        }
    }
    nPartials = count;

    return 0;
}


bool PlotDialog::getnDataCol()
{
    QString FileName;
    FileName = (baseFileName_+".EPSR.u01");
    QFile file(FileName);
    if(!file.open(QFile::ReadOnly | QFile::Text))
    {
        QMessageBox msgBox;
        msgBox.setText("Could not open .t01 file. This is required for plotting.");
        msgBox.exec();
        return 0;
    }
    QTextStream stream(&file);
    QString line;
    QStringList legendLine;
    line = stream.readLine();                                 //find dataset names and number of datasets - these are used in all other dataset based plots
    legendLine = line.split(" ", QString::SkipEmptyParts);
    nDatasets = legendLine.size()-2;
    nDataCol = nDatasets*2+1;
    file.close();
    return 0;
}


void PlotDialog::getplottype()
{
    plotFileExt_.clear();
    int ptType = ui.standardPlotList->currentRow();
    if (ptType != -1)
    {
        if (ptType == 0)
        {
            //F(Q)
            datasetPlot();
        }
        else
        if (ptType == 1)
        {
            //G(r)
            datasetPlot();
        }
        else
        if (ptType == 2)
        {
            //energy
            plotFileExt_ = ".EPSR.erg";
            dataColumn_ = 0;
            yPlot();
        }
        else
        if (ptType == 3)
        {
            //R-factor
            plotFileExt_ = ".EPSR.erg";
            dataColumn_= 2;
            yPlot();
        }
        else
        if (ptType == 4)
        {
            partialsplot();
        }
        else
        if (ptType == 5)
        {
            gofrintraplot();
        }
        else
        if (ptType == 6)
        {
            sqterplot();
        }
        else
        if (ptType == 7)
        {
            sqtraplot();
        }
        else
        if (ptType == 8)
        {
            //ereq plot
            ereqPlot();
        }
    }
    else
    {
        QString ptTypeName = ui.outputPlotList->currentItem()->text();
        if (ptTypeName.contains("CHAINS"))
        {
            histPlot();
              //column(3) (4th) also has some data in but not sure what??
        }
        else
        if (ptTypeName.contains("CLUSTER"))
        {
            histPlot();
        }
        else
        if (ptTypeName.contains("COORD"))
        {
            histPlot();
        }
        else
        if (ptTypeName.contains("FLUCTUATIONS"))
        {
            histPlot();
        }
        else
        if (ptTypeName.contains("RINGS"))
        {
            histPlot();
        }
        else
        if (ptTypeName.contains("TOR"))
        {
            histPlot();
        }
        else
        if (ptTypeName.contains("TRI"))
        {
            histPlot();
        }
        else
        if (ptTypeName.contains("VOIDS"))
        {
            histPlot();
        }
    }
}

bool PlotDialog::datasetPlot()
{
    QString modelFileName;
    QString dataFileName;
    QString diffFileName;
    QString xLabel;
    QString yLabel;
    double xMin;
    double yMin;
    double xMax;
    double yMax;

    //Filenames and number of datasets for F(Q) plot
    if (ui.standardPlotList->currentItem()->text().contains("F(Q)"))
    {
        modelFileName = (baseFileName_+".EPSR.u01");
        dataFileName = (baseFileName_+".EPSR.t01");
        diffFileName = (baseFileName_+".EPSR.v01");
        xLabel = "Q / Å\u207B\u00B9";
        yLabel = "F(Q)";
        xMin = 0.0;
        yMin = -1.5;
        xMax = 30.0;
        yMax = nDataCol;
    }
    else //for G(r) plot
    {
        modelFileName = (baseFileName_+".EPSR.x01");
        dataFileName = (baseFileName_+".EPSR.w01");
        diffFileName.clear();
        xLabel = "r / Å";
        yLabel = "G(r)";
        xMin = 0.0;
        yMin = -1.0;
        xMax = 20.0;
        yMax = nDataCol;
    }

    QFile fileM(modelFileName);
    QFile fileD(dataFileName);
    QFile fileDF(diffFileName);

    int column;
    double residualOffset = -0.2;
    double yZeroOffset = 0.0;
    double dataSetOffset = 1.0;
    if (!ui.residualOffsetLineEdit->text().isEmpty())
    {
        residualOffset = ui.residualOffsetLineEdit->text().toDouble();
    }  
    if (!ui.yZeroOffsetLineEdit->text().isEmpty())
    {
        yZeroOffset = ui.yZeroOffsetLineEdit->text().toDouble();
    }
    if (!ui.manOffsetLineEdit->text().isEmpty() && ui.manOffsetRB->isChecked())
    {
        dataSetOffset = ui.manOffsetLineEdit->text().toDouble();
    }
    double dataSetOffsetFactor = 1/dataSetOffset;

    //open and read data file to array
    if(!fileD.open(QFile::ReadOnly | QFile::Text))
    {
        QMessageBox msgBox;
        msgBox.setText("Could not open .t01 file.");
        msgBox.exec();
        return 0;
    }
    QTextStream streamD(&fileD);
    QString lineD;
    QStringList dataLineD;
    QVector<double> xD;
    QVector< QVector<double> > columnsD;
    dataLineD.clear();
    xD.clear();
    columnsD.clear();
    columnsD.resize(nDataCol);
    lineD = streamD.readLine();
    do
    {
        lineD = streamD.readLine();
        dataLineD = lineD.split(" ", QString::SkipEmptyParts);
        if (dataLineD.count() == 0) break;
        xD.append(dataLineD.at(0).toDouble());
        int nColumns = (dataLineD.count() - 1) / 2;
        for (column = 0; column < nColumns; ++column)
        {
            columnsD[column].append((dataLineD.at(column*2+1).toDouble())+column/dataSetOffsetFactor+yZeroOffset);
        }
    } while (!lineD.isNull());
    fileD.close();

    //open and read model file to array
    if(!fileM.open(QFile::ReadOnly | QFile::Text))
    {
        QMessageBox msgBox;
        msgBox.setText("Could not open .u01 file.");
        msgBox.exec();
        return 0;
    }
    QTextStream streamM(&fileM);
    QString lineM;
    QStringList dataLineM;
    QVector<double> xM;
    QVector< QVector<double> > columnsM;
    dataLineM.clear();
    xM.clear();
    columnsM.clear();
    columnsM.resize(nDataCol);
    lineM = streamM.readLine();
    do
    {
        lineM = streamM.readLine();
        dataLineM = lineM.split(" ", QString::SkipEmptyParts);
        if (dataLineM.count() == 0) break;
        xM.append(dataLineM.at(0).toDouble());
        int nColumns = (dataLineM.count() - 1) / 2;
        for (column = 0; column < nColumns; ++column)
        {
            columnsM[column].append((dataLineM.at(column*2+1).toDouble())+column/dataSetOffsetFactor+yZeroOffset);
        }
    } while (!lineM.isNull());
    fileM.close();

    QVector<double> xDF;
    QVector< QVector<double> > columnsDF;
    xDF.clear();
    columnsDF.clear();
    columnsDF.resize(nDataCol);
    //for F(Q) open and read difference file to array
    if (ui.standardPlotList->currentItem()->text().contains("F(Q)"))
    {
        if(!fileDF.open(QFile::ReadOnly | QFile::Text))
        {
            QMessageBox msgBox;
            msgBox.setText("Could not open .v01 file.");
            msgBox.exec();
            return 0;
        }
        QTextStream streamDF(&fileDF);
        QString lineDF;
        QStringList dataLineDF;
        dataLineDF.clear();
        lineDF = streamDF.readLine();
        do
        {
            lineDF = streamDF.readLine();
            dataLineDF = lineDF.split(" ", QString::SkipEmptyParts);
            if (dataLineDF.count() == 0) break;
            xDF.append(dataLineDF.at(0).toDouble());
            int nColumns = (dataLineDF.count() - 1) / 2;
            for (column = 0; column < nColumns; ++column)
            {
                columnsDF[column].append((dataLineDF.at(column*2+1).toDouble())+column/dataSetOffsetFactor+yZeroOffset+residualOffset);
            }
        } while (!lineDF.isNull());
        fileDF.close();
    }
    else //for G(r) create a difference array from model and data arrays
    {
        if (columnsM.count() != columnsD.count() || columnsM[0].count() != columnsD[0].count())
        {
            QMessageBox msgBox;
            msgBox.setText("The number of data points in the data and model files is not equal.");
            msgBox.exec();
            return 0;
        }
        for (int i = 0; i < columnsM[0].count(); i++)
        {
            xDF.append(xM.at(i));
            for (column = 0; column < nDataCol; ++column)
            {
                columnsDF[column].append(columnsM[column].at(i)-columnsD[column].at(i)+column/dataSetOffsetFactor+yZeroOffset+residualOffset); //diff = model-data -offset
            }
        }
    }

    //read x max to work out where to put data labels
    if (!ui.xMaxLineEdit->text().isEmpty())
    {
        xMax = ui.xMaxLineEdit->text().toDouble();
    }

    // create graph and assign data to it:
    QPen pen;
    QString datafileLabel;
    if (ui.residualCheckBox->isChecked())
    {
        for (int i=0; i < nDatasets*3; i += 3)
        {
            ui.customPlot->addGraph();
            pen.setColor(QColor(qSin(i*0.3)*100+100, qSin(i*0.6+0.7)*100+100, qSin(i*0.4+0.6)*100+100));
            ui.customPlot->graph(i)->setPen(pen);
            ui.customPlot->graph(i)->setData(xM, columnsM.at(i/3));
            QCPItemText *dataLabel = new QCPItemText(ui.customPlot);
            ui.customPlot->addItem(dataLabel);
            dataLabel->position->setCoords(0.7*xMax,((i/3)/dataSetOffsetFactor)+yZeroOffset+0.2);
            datafileLabel = mainWindow_->dataFileList.at(i/3);
            dataLabel->setText(qPrintable(datafileLabel));
            ui.customPlot->addGraph();
            ui.customPlot->graph(i+1)->setPen(pen);
            ui.customPlot->graph(i+1)->setData(xD, columnsD.at(i/3));
            ui.customPlot->graph(i+1)->setLineStyle(QCPGraph::lsNone);
            ui.customPlot->graph(i+1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 3));
            ui.customPlot->addGraph();
            ui.customPlot->graph(i+2)->setData(xDF, columnsDF.at(i/3));
            ui.customPlot->graph(i+2)->setPen(QPen(Qt::gray));
        }
    }
    else
    {
        for (int i=0; i < nDatasets*2; i += 2)
        {
            ui.customPlot->addGraph();
            pen.setColor(QColor(qSin(i*0.3)*100+100, qSin(i*0.6+0.7)*100+100, qSin(i*0.4+0.6)*100+100));
            ui.customPlot->graph(i)->setPen(pen);
            ui.customPlot->graph(i)->setData(xM, columnsM.at(i/2));
            QCPItemText *dataLabel = new QCPItemText(ui.customPlot);
            ui.customPlot->addItem(dataLabel);
            dataLabel->position->setCoords(0.7*xMax,((i/2)/dataSetOffsetFactor)+yZeroOffset+0.2);
            datafileLabel = mainWindow_->dataFileList.at(i/2);
            dataLabel->setText(qPrintable(datafileLabel));
            ui.customPlot->addGraph();
            ui.customPlot->graph(i+1)->setPen(pen);
            ui.customPlot->graph(i+1)->setData(xD, columnsD.at(i/2));
            ui.customPlot->graph(i+1)->setLineStyle(QCPGraph::lsNone);
            ui.customPlot->graph(i+1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 3));
        }
    }

    if (ui.logXButton->isChecked() == true)
    {
        ui.customPlot->xAxis->setScaleType(QCPAxis::stLogarithmic);
        ui.customPlot->xAxis->setScaleLogBase(100);
        ui.customPlot->xAxis->setNumberFormat("eb");
        ui.customPlot->xAxis->setNumberPrecision(0);
        ui.customPlot->xAxis->setSubTickCount(10);
    }
    else
    {
        ui.customPlot->xAxis->setScaleType(QCPAxis::stLinear);
        ui.customPlot->xAxis->setNumberFormat("g");
        ui.customPlot->xAxis->setNumberPrecision(2);
        ui.customPlot->xAxis->setAutoTickStep(true);
    }
    if (ui.logYButton->isChecked() == true)
    {
        ui.customPlot->yAxis->setScaleType(QCPAxis::stLogarithmic);
        ui.customPlot->yAxis->setScaleLogBase(100);
        ui.customPlot->yAxis->setNumberFormat("eb");
        ui.customPlot->yAxis->setNumberPrecision(0);
        ui.customPlot->yAxis->setSubTickCount(10);
    }
    else
    {
        ui.customPlot->yAxis->setScaleType(QCPAxis::stLinear);
        ui.customPlot->yAxis->setNumberFormat("g");
        ui.customPlot->yAxis->setNumberPrecision(2);
        ui.customPlot->yAxis->setAutoTickStep(true);
    }

    // give the axes some labels:
    ui.customPlot->xAxis->setLabel(xLabel);
    ui.customPlot->yAxis->setLabel(yLabel);

    //plot
    ui.customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
    if (ui.xMinLineEdit->text().isEmpty() && ui.xMaxLineEdit->text().isEmpty() && ui.yMinLineEdit->text().isEmpty() && ui.yMaxLineEdit->text().isEmpty())
    {
        ui.customPlot->rescaleAxes();
    }
    else
    {
        if (!ui.xMinLineEdit->text().isEmpty())
        {
            ui.customPlot->xAxis->setRangeLower(ui.xMinLineEdit->text().toDouble());
        }
        else
        ui.customPlot->xAxis->setRangeLower(xMin);

        if (!ui.xMaxLineEdit->text().isEmpty())
        {
            ui.customPlot->xAxis->setRangeUpper(ui.xMaxLineEdit->text().toDouble());
        }
        else
        ui.customPlot->xAxis->setRangeUpper(xMax);        //read this from data??

        if (!ui.yMinLineEdit->text().isEmpty())
        {
            ui.customPlot->yAxis->setRangeLower(ui.yMinLineEdit->text().toDouble());
        }
        else
        ui.customPlot->yAxis->setRangeLower(yMin);     //read this from data!!**************************************************

        if (!ui.yMaxLineEdit->text().isEmpty())
        {
            ui.customPlot->yAxis->setRangeUpper(ui.yMaxLineEdit->text().toDouble());
        }
        else
        ui.customPlot->yAxis->setRangeUpper(yMax);     //read this from data!!**************************************************
    }
    ui.customPlot->replot();
    return 0;
}

bool PlotDialog::yPlot()
{
    QString FileName;
    FileName = (baseFileName_+plotFileExt_);
    QFile file(FileName);

    //open and read data file to array
    if(!file.open(QFile::ReadOnly | QFile::Text))
    {
        QMessageBox msgBox;
        msgBox.setText("Could not open .erg file.");
        msgBox.exec();
        return 0;
    }
    QTextStream stream(&file);
    QString line;
    QStringList dataLine;
    QVector<double> x;          //number of iterations
    QVector<double> y;          //y value
    dataLine.clear();
    x.clear();
    y.clear();
    int iterations = 1;
    do {
        line = stream.readLine();
        dataLine = line.split(" ", QString::SkipEmptyParts);
        if (dataLine.count() == 0) break;
        y.append(dataLine.at(dataColumn_).toDouble());
        x.append(iterations);
        iterations++;
    } while (!line.isNull());
    file.close();

    // create graph and assign data to it:
    ui.customPlot->addGraph();
    ui.customPlot->graph()->setData(x, y);
    if (ui.logXButton->isChecked() == true)
    {
        ui.customPlot->xAxis->setScaleType(QCPAxis::stLogarithmic);
        ui.customPlot->xAxis->setScaleLogBase(100);
        ui.customPlot->xAxis->setNumberFormat("eb");
        ui.customPlot->xAxis->setNumberPrecision(0);
        ui.customPlot->xAxis->setSubTickCount(10);
    }
    else
    {
        ui.customPlot->xAxis->setScaleType(QCPAxis::stLinear);
        ui.customPlot->xAxis->setNumberFormat("g");
        ui.customPlot->xAxis->setNumberPrecision(2);
        ui.customPlot->xAxis->setAutoTickStep(true);
    }
    if (ui.logYButton->isChecked() == true)
    {
        ui.customPlot->yAxis->setScaleType(QCPAxis::stLogarithmic);
        ui.customPlot->yAxis->setScaleLogBase(100);
        ui.customPlot->yAxis->setNumberFormat("eb");
        ui.customPlot->yAxis->setNumberPrecision(0);
        ui.customPlot->yAxis->setSubTickCount(10);
    }
    else
    {
        ui.customPlot->yAxis->setScaleType(QCPAxis::stLinear);
        ui.customPlot->yAxis->setNumberFormat("g");
        ui.customPlot->yAxis->setNumberPrecision(2);
        ui.customPlot->yAxis->setAutoTickStep(true);
    }

    // give the axes some labels:
    ui.customPlot->xAxis->setLabel("iteration");
    if (dataColumn_ == 0)
    {
        ui.customPlot->yAxis->setLabel("Energy");
    }
    else
    if (dataColumn_ == 2)
    {
        ui.customPlot->yAxis->setLabel("R-factor");
    }

    //plot
    ui.customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
    ui.customPlot->rescaleAxes();
    ui.customPlot->replot();
    return 0;
}

bool PlotDialog::histPlot()
{
    QString FileName = workingDir_+ui.outputPlotList->currentItem()->text();
    QFile file(FileName);
    int atomSet = ui.outputAtomSetList->currentRow();
    if (ui.outputAtomSetList->currentItem()->text().contains("CHAINS"))
    {
        atomSet = atomSet*4+1;
    }
    else
    {
        atomSet = atomSet*2+1;
    }

    //open and read data file to array
    if(!file.open(QFile::ReadOnly | QFile::Text))
    {
        QMessageBox msgBox;
        msgBox.setText("Could not open "+FileName);
        msgBox.exec();
        return 0;
    }
    QTextStream stream(&file);
    QString line;
    QStringList dataLine;
    QVector<double> x;
    QVector<double> y;
    dataLine.clear();
    x.clear();
    y.clear();
    line = stream.readLine();
    do {
        line = stream.readLine();
        dataLine = line.split(" ", QString::SkipEmptyParts);
        if (dataLine.count() == 0) break;
        x.append(dataLine.at(0).toDouble());
        y.append(dataLine.at(atomSet).toDouble());
    } while (!line.isNull());
    file.close();

    // create graph and assign data to it:
    QCPBars *histogram = new QCPBars(ui.customPlot->xAxis, ui.customPlot->yAxis);
    ui.customPlot->addPlottable(histogram);
    histogram->setData(x, y);
    if (ui.logXButton->isChecked() == true)
    {
        ui.customPlot->xAxis->setScaleType(QCPAxis::stLogarithmic);
        ui.customPlot->xAxis->setScaleLogBase(100);
        ui.customPlot->xAxis->setNumberFormat("eb");
        ui.customPlot->xAxis->setNumberPrecision(0);
        ui.customPlot->xAxis->setSubTickCount(10);
    }
    else
    {
        ui.customPlot->xAxis->setScaleType(QCPAxis::stLinear);
        ui.customPlot->xAxis->setNumberFormat("g");
        ui.customPlot->xAxis->setNumberPrecision(3);
        ui.customPlot->xAxis->setAutoTickStep(true);
    }
    if (ui.logYButton->isChecked() == true)
    {
        ui.customPlot->yAxis->setScaleType(QCPAxis::stLogarithmic);
        ui.customPlot->yAxis->setScaleLogBase(100);
        ui.customPlot->yAxis->setNumberFormat("eb");
        ui.customPlot->yAxis->setNumberPrecision(0);
        ui.customPlot->yAxis->setSubTickCount(10);
    }
    else
    {
        ui.customPlot->yAxis->setScaleType(QCPAxis::stLinear);
        ui.customPlot->yAxis->setNumberFormat("g");
        ui.customPlot->yAxis->setNumberPrecision(3);
        ui.customPlot->yAxis->setAutoTickStep(true);
    }

    // give the axes some labels:
    if (FileName.contains("CHAINS"))
    {
        ui.customPlot->xAxis->setLabel("Chain size");
        ui.customPlot->yAxis->setLabel("Percentage");
    }
    else
    if (FileName.contains("VOIDS"))
    {
        ui.customPlot->xAxis->setLabel("Void size");
        ui.customPlot->yAxis->setLabel("Probability");
    }
    else
    if (FileName.contains("RINGS"))
    {
        ui.customPlot->xAxis->setLabel("Ring size");
        ui.customPlot->yAxis->setLabel("Probability");
    }
    else
    if (FileName.contains("FLUCTUATIONS"))
    {
        ui.customPlot->xAxis->setLabel("Fluctuation size");
        ui.customPlot->yAxis->setLabel("Probability");
    }
    else
    if (FileName.contains("CLUSTERS"))
    {
        ui.customPlot->xAxis->setLabel("Cluster size");
        ui.customPlot->yAxis->setLabel("Probability");
    }
    else
    if (FileName.contains("COORD"))
    {
        ui.customPlot->xAxis->setLabel("Coordination number");
        ui.customPlot->yAxis->setLabel("Probability");
    }
    else
    {
        ui.customPlot->xAxis->setLabel("Theta / °");
        ui.customPlot->yAxis->setLabel("Probability");
    }

    //plot
    ui.customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
    ui.customPlot->rescaleAxes();
    ui.customPlot->replot();
    return 0;
}

bool PlotDialog::Pplot()
{
    QString FileName;
    FileName = (baseFileName_+".EPSR.erg");
    QFile file(FileName);

    //open and read data file to array
    if(!file.open(QFile::ReadOnly | QFile::Text))
    {
        QMessageBox msgBox;
        msgBox.setText("Could not open .erg file.");
        msgBox.exec();
        return 0;
    }
    QTextStream stream(&file);
    QString line;
    QStringList dataLine;
    QVector<double> x;          //number of iterations
    QVector<double> y;          //pressure
    dataLine.clear();
    x.clear();
    y.clear();
    for (int iterations = 1; iterations < 1000000; ++iterations)
    {
        line = stream.readLine();
        dataLine = line.split(" ", QString::SkipEmptyParts);
        if (dataLine.count() == 0) break;
        y.append(dataLine.at(1).toDouble());
        x.append(iterations);
    }
    file.close();

    // create graph and assign data to it:
    ui.customPlot->addGraph();
    ui.customPlot->graph()->setData(x, y);
    if (ui.logXButton->isChecked() == true)
    {
        ui.customPlot->xAxis->setScaleType(QCPAxis::stLogarithmic);
        ui.customPlot->xAxis->setScaleLogBase(100);
        ui.customPlot->xAxis->setNumberFormat("eb");
        ui.customPlot->xAxis->setNumberPrecision(0);
        ui.customPlot->xAxis->setSubTickCount(10);
    }
    else
    {
        ui.customPlot->xAxis->setScaleType(QCPAxis::stLinear);
        ui.customPlot->xAxis->setNumberFormat("g");
        ui.customPlot->xAxis->setNumberPrecision(2);
        ui.customPlot->xAxis->setAutoTickStep(true);
    }
    if (ui.logYButton->isChecked() == true)
    {
        ui.customPlot->yAxis->setScaleType(QCPAxis::stLogarithmic);
        ui.customPlot->yAxis->setScaleLogBase(100);
        ui.customPlot->yAxis->setNumberFormat("eb");
        ui.customPlot->yAxis->setNumberPrecision(0);
        ui.customPlot->yAxis->setSubTickCount(10);
    }
    else
    {
        ui.customPlot->yAxis->setScaleType(QCPAxis::stLinear);
        ui.customPlot->yAxis->setNumberFormat("g");
        ui.customPlot->yAxis->setNumberPrecision(2);
        ui.customPlot->yAxis->setAutoTickStep(true);
    }

    // give the axes some labels:
    ui.customPlot->xAxis->setLabel("iteration");
    ui.customPlot->yAxis->setLabel("Pressure");

    //plot
    ui.customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
    ui.customPlot->rescaleAxes();
    ui.customPlot->replot();
    return 0;
}

bool PlotDialog::Ereqplot()
{
    QString FileName;
    FileName = (baseFileName_+".EPSR.erg");
    QFile file(FileName);

    //open and read data file to array
    if(!file.open(QFile::ReadOnly | QFile::Text))
    {
        QMessageBox msgBox;
        msgBox.setText("Could not open .erg file.");
        msgBox.exec();
        return 0;
    }
    QTextStream stream(&file);
    QString line;
    QStringList dataLine;
    QVector<double> x1;          //ereq
    QVector<double> y1;         //fit quality
    QVector<double> x2;          //fitted line x values
    QVector<double> y2;         //fitted line showing gradient of last 50 iterations
    dataLine.clear();
    x1.clear();
    x2.clear();
    y1.clear();
    y2.clear();
    for (int iterations = 1; iterations < 1000000; ++iterations)
    {
        line = stream.readLine();
        dataLine = line.split(" ", QString::SkipEmptyParts);
        if (dataLine.count() == 0) break;
        if (dataLine.count() <= 6)
        {
            QMessageBox msgBox;
            msgBox.setText("This plot type is not compatible with this version of EPSR.");
            msgBox.exec();
            file.close();
            return 0;
        }
        x1.append(dataLine.at(5).toDouble());
        y1.append(dataLine.at(6).toDouble());
        x2.append(dataLine.at(7).toDouble());
        y2.append(dataLine.at(8).toDouble());
    }
    file.close();

    // create graph and assign data to it:
    ui.customPlot->addGraph();
    ui.customPlot->graph()->setData(x1, y1);
    ui.customPlot->addGraph();
    ui.customPlot->graph(1)->setData(x2, y2);
    ui.customPlot->graph(1)->setLineStyle(QCPGraph::lsNone);
    ui.customPlot->graph(1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 3));

    // give the axes some labels:
    ui.customPlot->xAxis->setLabel("ereq energy");
    ui.customPlot->yAxis->setLabel("quality of fit");

    //plot
    ui.customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
    ui.customPlot->rescaleAxes();
    ui.customPlot->replot();
    return 0;
}

bool PlotDialog::partialsplot()
{
    //this is the number of pairs to be plotted
    int numberPairs = iList.count();

    //this is the QVector of QVectors where the data from the pairs listed above will be stored
    QVector< QVector<double> > columns;
    columns.clear();
    columns.resize(numberPairs+1);
    QVector< QVector<double> > columnsCN;
    columnsCN.clear();
    columnsCN.resize(numberPairs+1);
    QVector<double> x;

    //read the data for each pair in the list from the data file by looping over each pair in the list
    for (int pair = 0; pair < numberPairs; ++pair)
    {
        int i = iList.at(pair); // this is the row of the first atom of the pair
        int j = jList.at(pair); // this is the row of the second atom in the pair
        int atompair_index = ij.ref(i,j);
        QString partialsFileName;
        QString CNFileName;

        //the first file contains 100 pairs (even columns from 2 to 200) and atompair_indexes from 0 - 99
        if (atompair_index < 100)
        {
            if (ui.outputPlotList->currentRow() != -1)
            {
                QString baseFileName = ui.outputPlotList->currentItem()->text().split(".", QString::KeepEmptyParts).at(0);
                partialsFileName = workingDir_+baseFileName+".PARTIALS.g01";
            }
            else {  partialsFileName = (baseFileName_+".EPSR.g01");}
            CNFileName = (baseFileName_+".EPSR.z01");
        }
        else
        if (99 < atompair_index && atompair_index < 200)
        {
            partialsFileName = (baseFileName_+".EPSR.g02");
            CNFileName = (baseFileName_+".EPSR.z02");
            atompair_index = atompair_index-100;
        }
        else
        if (199 < atompair_index && atompair_index < 300)
        {
            partialsFileName = (baseFileName_+".EPSR.g03");
            CNFileName = (baseFileName_+".EPSR.z03");
            atompair_index = atompair_index-200;
        }
        else
        if (299 < atompair_index && atompair_index < 400)
        {
            partialsFileName = (baseFileName_+".EPSR.g04");
            CNFileName = (baseFileName_+".EPSR.z04");
            atompair_index = atompair_index-300;
        }
        else
        if (399 < atompair_index && atompair_index < 500)
        {
            partialsFileName = (baseFileName_+".EPSR.g05");
            CNFileName = (baseFileName_+".EPSR.z05");
            atompair_index = atompair_index-400;
        }

        //the first atom pair index = 0 therefore need to x2 and +1 in order to get to correct column.
        int CNpair_column = atompair_index*2+1;
        if (i > j)
        {
            CNpair_column++;
        }

        QFile file(partialsFileName);
        if(!file.open(QFile::ReadOnly | QFile::Text))
        {
            QMessageBox msgBox;
            msgBox.setText("Could not open .g0# file.");
            msgBox.exec();
            return 0;
        }
        QTextStream stream(&file);
        QString line;
        QStringList dataLine;
        dataLine.clear();
        x.clear();
        line = stream.readLine();
        do
        {
            line = stream.readLine();
            dataLine = line.split(" ", QString::SkipEmptyParts);
            if (dataLine.count() == 0) break;
            x.append(dataLine.at(0).toDouble());
            columns[pair].append(dataLine.at(atompair_index*2+1).toDouble()+pair);
        } while (!line.isNull());
        file.close();

        QFile fileCN(CNFileName);
        if(!fileCN.open(QFile::ReadOnly | QFile::Text))
        {
            QMessageBox msgBox;
            msgBox.setText("Could not open .z0# file.");
            msgBox.exec();
            return 0;
        }
        QTextStream streamCN(&fileCN);
        QString lineCN;
        QStringList dataLineCN;
        dataLineCN.clear();
        lineCN = streamCN.readLine();
        do
        {
            lineCN = streamCN.readLine();
            dataLineCN = lineCN.split(" ", QString::SkipEmptyParts);
            if (dataLineCN.count() == 0) break;
            columnsCN[pair].append(dataLineCN.at(CNpair_column).toDouble()+pair);
        } while (!lineCN.isNull());
        fileCN.close();
    }

    // create graph and assign data to it:
    if (ui.CNcheckBox->isChecked())
    {
        QPen pen;
        QString pairLabel;
        for (int i=0; i < numberPairs*2; i += 2)
        {
            ui.customPlot->addGraph();
            pen.setColor(QColor(qSin(i*0.3)*100+100, qSin(i*0.6+0.7)*100+100, qSin(i*0.4+0.6)*100+100));
            ui.customPlot->graph(i)->setPen(pen);
            ui.customPlot->graph(i)->setData(x, columns.at(i/2));
            QCPItemText *dataLabel = new QCPItemText(ui.customPlot);
            ui.customPlot->addItem(dataLabel);
            dataLabel->position->setCoords(12,(i/2)+1.1);
            pairLabel = ui.pairStackPlotList->item(i/2)->text();
            dataLabel->setText(qPrintable(pairLabel));
            ui.customPlot->addGraph();
            ui.customPlot->graph(i+1)->setPen(QPen(Qt::gray));
            ui.customPlot->graph(i+1)->setData(x, columnsCN.at(i/2));
        }
    }
    else
    {
        QPen pen;
        QString pairLabel;
        for (int i=0; i < numberPairs; ++i)
        {
            ui.customPlot->addGraph();
            pen.setColor(QColor(qSin(i*0.3)*100+100, qSin(i*0.6+0.7)*100+100, qSin(i*0.4+0.6)*100+100));
            ui.customPlot->graph(i)->setPen(pen);
            ui.customPlot->graph(i)->setData(x, columns.at(i));
            QCPItemText *dataLabel = new QCPItemText(ui.customPlot);
            ui.customPlot->addItem(dataLabel);
            dataLabel->position->setCoords(12,i+1.1);
            pairLabel = ui.pairStackPlotList->item(i)->text();
            dataLabel->setText(qPrintable(pairLabel));
        }
    }

    // give the axes some labels:
    ui.customPlot->xAxis->setLabel("r (Angstrom)");
    ui.customPlot->yAxis->setLabel("g(r)");

    ui.customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
    ui.customPlot->xAxis->setRange(0, 15);
    ui.customPlot->yAxis->setRange(0, numberPairs+1);
//    ui.customPlot->rescaleAxes();
    ui.customPlot->replot();
    return 0;
}

bool PlotDialog::gofrintraplot()
{
    //this is the number of pairs to be plotted
    int numberPairs = iList.count();

    //this is the QVector of QVectors where the data from the pairs listed above will be stored
    QVector< QVector<double> > columns;
    columns.clear();
    columns.resize(numberPairs+1);
    QVector<double> x;

    //read the data for each pair in the list from the data file by looping over each pair in the list
    for (int pair = 0; pair < numberPairs; ++pair)
    {
        int i = iList.at(pair); // this is the row of the first atom of the pair
        int j = jList.at(pair); // this is the row of the second atom in the pair
        int atompair_index = ij.ref(i,j);

        QString partialsFileName;

        if (atompair_index < 100)
        {
            partialsFileName = (baseFileName_+".EPSR.y01");
        }
        else
        if (99 < atompair_index && atompair_index < 200)
        {
            partialsFileName = (baseFileName_+".EPSR.y02");
            atompair_index = atompair_index-100;
        }
        else
        if (199 < atompair_index && atompair_index < 300)
        {
            partialsFileName = (baseFileName_+".EPSR.y03");
            atompair_index = atompair_index-200;
        }
        else
        if (299 < atompair_index && atompair_index < 400)
        {
            partialsFileName = (baseFileName_+".EPSR.y04");
            atompair_index = atompair_index-300;
        }
        else
        if (399 < atompair_index && atompair_index < 500)
        {
            partialsFileName = (baseFileName_+".EPSR.y05");
            atompair_index = atompair_index-400;
        }
        else
        if (499 < atompair_index && atompair_index < 600)
        {
            partialsFileName = (baseFileName_+".EPSR.y06");
            atompair_index = atompair_index-500;
        }
        else
        if (599 < atompair_index && atompair_index < 700)
        {
            partialsFileName = (baseFileName_+".EPSR.y07");
            atompair_index = atompair_index-600;
        }
        else
        if (699 < atompair_index && atompair_index < 800)
        {
            partialsFileName = (baseFileName_+".EPSR.y08");
            atompair_index = atompair_index-700;
        }
        else
        if (799 < atompair_index && atompair_index < 900)
        {
            partialsFileName = (baseFileName_+".EPSR.y09");
            atompair_index = atompair_index-800;
        }
        else
        if (899 < atompair_index && atompair_index < 1000)
        {
            partialsFileName = (baseFileName_+".EPSR.y10");
            atompair_index = atompair_index-900;
        }

        QFile file(partialsFileName);
        if(!file.open(QFile::ReadOnly | QFile::Text))
        {
            QMessageBox msgBox;
            msgBox.setText("Could not open .y0# file.");
            msgBox.exec();
            return 0;
        }
        QTextStream stream(&file);
        QString line;
        QStringList dataLine;
        dataLine.clear();
        x.clear();
        line = stream.readLine();
        do
        {
            line = stream.readLine();
            dataLine = line.split(" ", QString::SkipEmptyParts);
            if (dataLine.count() == 0) break;
            x.append(dataLine.at(0).toDouble());
            columns[pair].append(dataLine.at(atompair_index*2+1).toDouble()+pair);
        } while (!line.isNull());
        file.close();
    }

    // create graph and assign data to it:
    QPen pen;
    QString pairLabel;
    for (int i=0; i < numberPairs; ++i)
    {
        ui.customPlot->addGraph();
        pen.setColor(QColor(qSin(i*0.3)*100+100, qSin(i*0.6+0.7)*100+100, qSin(i*0.4+0.6)*100+100));
        ui.customPlot->graph(i)->setPen(pen);
        ui.customPlot->graph(i)->setData(x, columns.at(i));
        QCPItemText *dataLabel = new QCPItemText(ui.customPlot);
        ui.customPlot->addItem(dataLabel);
        dataLabel->position->setCoords(12,i+0.1);
        pairLabel = ui.pairStackPlotList->item(i)->text();
        dataLabel->setText(qPrintable(pairLabel));
    }

    // give the axes some labels:
    ui.customPlot->xAxis->setLabel("r (Angstrom)");
    ui.customPlot->yAxis->setLabel("g(r)");

    ui.customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
    ui.customPlot->rescaleAxes();
//    ui.customPlot->xAxis->setRange(0, 15);
//    ui.customPlot->yAxis->setRange(0, numberPairs+1);
    ui.customPlot->replot();
    return 0;
}

bool PlotDialog::sqterplot()
{
    //this is the number of pairs to be plotted
    int numberPairs = iList.count();

    //this is the QVector of QVectors where the data from the pairs listed above will be stored
    QVector< QVector<double> > columns;
    columns.clear();
    columns.resize(numberPairs+1);
    QVector<double> x;

    //read the data for each pair in the list from the data file by looping over each pair in the list
    for (int pair = 0; pair < numberPairs; ++pair)
    {
        int i = iList.at(pair); // this is the row of the first atom of the pair
        int j = jList.at(pair); // this is the row of the second atom in the pair
        int atompair_index = ij.ref(i,j);

        QString partialsFileName;

        if (atompair_index < 100)
        {
            partialsFileName = (baseFileName_+".EPSR.f01");
        }
        else
        if (99 < atompair_index && atompair_index < 200)
        {
            partialsFileName = (baseFileName_+".EPSR.f02");
            atompair_index = atompair_index-100;
        }
        else
        if (199 < atompair_index && atompair_index < 300)
        {
            partialsFileName = (baseFileName_+".EPSR.f03");
            atompair_index = atompair_index-200;
        }
        else
        if (299 < atompair_index && atompair_index < 400)
        {
            partialsFileName = (baseFileName_+".EPSR.f04");
            atompair_index = atompair_index-300;
        }
        else
        if (399 < atompair_index && atompair_index < 500)
        {
            partialsFileName = (baseFileName_+".EPSR.f05");
            atompair_index = atompair_index-400;
        }

        QFile file(partialsFileName);
        if(!file.open(QFile::ReadOnly | QFile::Text))
        {
            QMessageBox msgBox;
            msgBox.setText("Could not open .f0# file.");
            msgBox.exec();
            return 0;
        }
        QTextStream stream(&file);
        QString line;
        QStringList dataLine;
        dataLine.clear();
        x.clear();
        line = stream.readLine();
        do
        {
            line = stream.readLine();
            dataLine = line.split(" ", QString::SkipEmptyParts);
            if (dataLine.count() == 0) break;
            x.append(dataLine.at(0).toDouble());
            columns[pair].append(dataLine.at(atompair_index*2+1).toDouble()+pair);
        } while (!line.isNull());
        file.close();
    }

    // create graph and assign data to it:
    QPen pen;
    QString pairLabel;
    for (int i=0; i < numberPairs; ++i)
    {
        ui.customPlot->addGraph();
        pen.setColor(QColor(qSin(i*0.3)*100+100, qSin(i*0.6+0.7)*100+100, qSin(i*0.4+0.6)*100+100));
        ui.customPlot->graph(i)->setPen(pen);
        ui.customPlot->graph(i)->setData(x, columns.at(i));
        QCPItemText *dataLabel = new QCPItemText(ui.customPlot);
        ui.customPlot->addItem(dataLabel);
        dataLabel->position->setCoords(18,i+0.1);
        pairLabel = ui.pairStackPlotList->item(i)->text();
        dataLabel->setText(qPrintable(pairLabel));
    }

    // give the axes some labels:
    ui.customPlot->xAxis->setLabel("r (Angstrom)");
    ui.customPlot->yAxis->setLabel("g(r)");

    ui.customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
//    ui.customPlot->xAxis->setRange(0, 15);
//    ui.customPlot->yAxis->setRange(0, numberPairs+1);
    ui.customPlot->rescaleAxes();
    ui.customPlot->replot();
    return 0;

}

bool PlotDialog::sqtraplot()
{
    //this is the number of pairs to be plotted
    int numberPairs = iList.count();

    //this is the QVector of QVectors where the data from the pairs listed above will be stored
    QVector< QVector<double> > columns;
    columns.clear();
    columns.resize(numberPairs+1);
    QVector<double> x;

    //read the data for each pair in the list from the data file by looping over each pair in the list
    for (int pair = 0; pair < numberPairs; ++pair)
    {
        int i = iList.at(pair); // this is the row of the first atom of the pair
        int j = jList.at(pair); // this is the row of the second atom in the pair
        int atompair_index = ij.ref(i,j);

        QString partialsFileName;

        if (atompair_index < 100)
        {
            partialsFileName = (baseFileName_+".EPSR.s01");
        }
        else
        if (99 < atompair_index && atompair_index < 200)
        {
            partialsFileName = (baseFileName_+".EPSR.s02");
            atompair_index = atompair_index-100;
        }
        else
        if (199 < atompair_index && atompair_index < 300)
        {
            partialsFileName = (baseFileName_+".EPSR.s03");
            atompair_index = atompair_index-200;
        }
        else
        if (299 < atompair_index && atompair_index < 400)
        {
            partialsFileName = (baseFileName_+".EPSR.s04");
            atompair_index = atompair_index-300;
        }
        else
        if (399 < atompair_index && atompair_index < 500)
        {
            partialsFileName = (baseFileName_+".EPSR.s05");
            atompair_index = atompair_index-400;
        }

        QFile file(partialsFileName);
        if(!file.open(QFile::ReadOnly | QFile::Text))
        {
            QMessageBox msgBox;
            msgBox.setText("Could not open .s0# file.");
            msgBox.exec();
            return 0;
        }
        QTextStream stream(&file);
        QString line;
        QStringList dataLine;
        dataLine.clear();
        x.clear();
        line = stream.readLine();
        do
        {
            line = stream.readLine();
            dataLine = line.split(" ", QString::SkipEmptyParts);
            if (dataLine.count() == 0) break;
            x.append(dataLine.at(0).toDouble());
            columns[pair].append(dataLine.at(atompair_index*2+1).toDouble()+pair);
        } while (!line.isNull());
        file.close();
    }

    // create graph and assign data to it:
    QPen pen;
    QString pairLabel;
    for (int i=0; i < numberPairs; ++i)
    {
        ui.customPlot->addGraph();
        pen.setColor(QColor(qSin(i*0.3)*100+100, qSin(i*0.6+0.7)*100+100, qSin(i*0.4+0.6)*100+100));
        ui.customPlot->graph(i)->setPen(pen);
        ui.customPlot->graph(i)->setData(x, columns.at(i));
        QCPItemText *dataLabel = new QCPItemText(ui.customPlot);
        ui.customPlot->addItem(dataLabel);
        dataLabel->position->setCoords(18,(i)+0.1);
        pairLabel = ui.pairStackPlotList->item(i)->text();
        dataLabel->setText(qPrintable(pairLabel));
    }

    // give the axes some labels:
    ui.customPlot->xAxis->setLabel("q (Angstrom-1)");
    ui.customPlot->yAxis->setLabel("s(q)");

    ui.customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
//    ui.customPlot->xAxis->setRange(0, 15);
//    ui.customPlot->yAxis->setRange(0, numberPairs+1);
    ui.customPlot->rescaleAxes();
    ui.customPlot->replot();
    return 0;
}

bool PlotDialog::ereqPlot()
{
    QString FileNameerg;
    FileNameerg = (baseFileName_+".EPSR.erg");
    QFile fileerg(FileNameerg);

    //open and read data file to array
    if(!fileerg.open(QFile::ReadOnly | QFile::Text))
    {
        QMessageBox msgBox;
        msgBox.setText("Could not open .erg file.");
        msgBox.exec();
        return 0;
    }
    QTextStream streamerg(&fileerg);
    QString lineerg;
    QStringList dataLineerg;
    QVector<double> x1;          //ereq
    QVector<double> y1;         //fit quality
    QVector<double> y2;         //fitted line showing gradient of last 50 iterations
    dataLineerg.clear();
    x1.clear();
    y1.clear();
    y2.clear();
    for (int iterations = 1; iterations < 1000000; ++iterations)
    {
        lineerg = streamerg.readLine();
        dataLineerg = lineerg.split(" ", QString::SkipEmptyParts);
        if (dataLineerg.count() == 0) break;
        if (dataLineerg.count() <= 6)
        {
            QMessageBox msgBox;
            msgBox.setText("This plot type is not compatible with this version of EPSR.");
            msgBox.exec();
            fileerg.close();
            return 0;
        }
        x1.append(dataLineerg.at(3).toDouble());
        y1.append(dataLineerg.at(6).toDouble());
    }
    fileerg.close();

    QString FileNameqdr;
    FileNameqdr = (baseFileName_+".EPSR.qdr");
    QFile fileqdr(FileNameqdr);

    //open and read data file to array
    if(!fileqdr.open(QFile::ReadOnly | QFile::Text))
    {
        QMessageBox msgBox;
        msgBox.setText("Could not open .erg file.");
        msgBox.exec();
        return 0;
    }
    QTextStream streamqdr(&fileqdr);
    QString lineqdr;
    QStringList dataLineqdr;
    dataLineqdr.clear();

    for (int iterations = 1; iterations < 1000000; ++iterations)
    {
        lineqdr = streamqdr.readLine();
        dataLineqdr = lineqdr.split(" ", QString::SkipEmptyParts);
        if (dataLineqdr.count() == 0) break;
        if (dataLineqdr.count() <= 6)
        {
            QMessageBox msgBox;
            msgBox.setText("This plot type is not compatible with this version of EPSR.");
            msgBox.exec();
            fileqdr.close();
            return 0;
        }
        y2.append(dataLineqdr.at(4).toDouble());
    }
    fileqdr.close();

    // create graph and assign data to it:
    QCPCurve *ergCurve = new QCPCurve(ui.customPlot->xAxis, ui.customPlot->yAxis);
    ui.customPlot->addPlottable(ergCurve);
    ergCurve->setData(x1, y1);
    ui.customPlot->addGraph();
    ui.customPlot->graph(0)->setData(x1, y2);
    ui.customPlot->graph(0)->setLineStyle(QCPGraph::lsNone);
    ui.customPlot->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 3));

    // give the axes some labels:
    ui.customPlot->xAxis->setLabel("ereq energy");
    ui.customPlot->yAxis->setLabel("quality of fit");

    //plot
    ui.customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
    ui.customPlot->rescaleAxes();
    ui.customPlot->replot();
    return 0;
}

void PlotDialog::on_addPairButton_clicked(bool checked)
{
    QString atom1 = ui.listWidget->currentItem()->text();
    QString atom2 = ui.listWidget_2->currentItem()->text();

    QString atomPair = atom1+"-"+atom2;

    ui.pairStackPlotList->addItem(atomPair);

    iList.append(ui.listWidget->currentRow());
    jList.append(ui.listWidget_2->currentRow());

}

void PlotDialog::on_removePairButton_clicked(bool checked)
{
    int row = ui.pairStackPlotList->currentRow();
    if (ui.pairStackPlotList->count() > 0)
    {
        ui.pairStackPlotList->takeItem(row);
        iList.removeAt(row);
        jList.removeAt(row);
    }
}

void PlotDialog::on_optionsButton_clicked(bool checked)
{
    if (ui.optionsGroupBox->isVisible() == true)
    {
        ui.optionsGroupBox->setVisible(false);
    }
    else
    {
        ui.optionsGroupBox->setVisible(true);
    }
}

void PlotDialog::on_savePlotButton_clicked(bool checked)
{
    QString plotFile = QFileDialog::getSaveFileName(this, "Input a filename and choose the file type", workingDir_, tr(".jpg files (*.jpg);;.png files(*.png);;.bmp files (*.bmp)"));
    if (!plotFile.isEmpty())
    {
        QString fileExt = plotFile.split(".", QString::SkipEmptyParts).at(1);
        if (fileExt.contains("jpg"))
        {
            ui.customPlot->savePng(plotFile, 0, 0, 1.0, -1);
        }
        else
        if (fileExt.contains("png"))
        {
            ui.customPlot->saveJpg(plotFile, 0, 0, 1.0, -1);
        }
        else
        if (fileExt.contains("bmp"))
        {
            ui.customPlot->saveBmp(plotFile, 0, 0, 1.0);
        }
    }
}
