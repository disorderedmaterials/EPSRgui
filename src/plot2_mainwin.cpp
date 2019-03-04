#include "qcustomplot.h"
#include "epsrproject.h"

#include <QVector>
#include <QWidget>
#include <QtGui>
#include <QFile>

void MainWindow::on_plot2Button_clicked()
{
    plot2();
}

void MainWindow::plot2()
{
    ui.plot2->clearGraphs();
    ui.plot2->clearItems();
    ui.plot2->clearPlottables();
    ui.plot2->axisRect()->setupFullAxesBox();
    ui.plot2->xAxis2->setVisible(false);
    ui.plot2->yAxis2->setVisible(false);
    getplottype2();
}

bool MainWindow::getplottype2()
{
    int ptType = ui.plotComboBox2->currentIndex();
    if (ptType == 0)
    {
        fqplot2();
        return 0;
    }
    else
    if (ptType == 1)
    {
        frplot2();
        return 0;
    }
    else
    if (ptType == 2)
    {
        Eplot2();
        return 0;
    }
    else
    if (ptType == 3)
    {
        Rplot2();
        return 0;
    }
    else
    if (ptType == 4)

    {
        Pplot2();
        return 0;
    }
    if (ptType == 5)

    {
        Ereqplot2();
        return 0;
    }
    else
    return 0;
}

bool MainWindow::fqplot2()
{   
    //Filenames and number of datasets
    if (dataFileList.isEmpty()) return 0;
    int nDatasets = dataFileList.count();
    int nDataCol = nDatasets*2+1;

    baseFileName_= (workingDir_+epsrInpFileName_).split(".").at(0);
    QString fqmodelFileName;
    fqmodelFileName = (baseFileName_+".EPSR.u01");
    QString fqdataFileName;
    fqdataFileName = (baseFileName_+".EPSR.t01");
    QString fqdiffFileName;
    fqdiffFileName = (baseFileName_+".EPSR.v01");
    QFile fileM(fqmodelFileName);
    QFile fileD(fqdataFileName);
    QFile fileDF(fqdiffFileName);
    int column = 0;

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
    QVector< QVector<double> > xD;
    QVector< QVector<double> > columnsD;
    dataLineD.clear();
    xD.clear();
    columnsD.clear();
    xD.resize(nDataCol);
    columnsD.resize(nDataCol);
    int nColumns = 0;
    lineD = streamD.readLine();
    do
    {
        lineD = streamD.readLine();
        dataLineD = lineD.split(" ", QString::SkipEmptyParts);
        if (dataLineD.count() == 0) break;
        nColumns = (dataLineD.count() - 1) / 2;
        for (column = 0; column < nColumns; ++column)
        {
            double yval = dataLineD.at(column*2+1).toDouble();
            if (yval != 0.0)
            {
                xD[column].append(dataLineD.at(0).toDouble());
                if (ui.plot2LogY->isChecked() == true)
                {
                    columnsD[column].append((dataLineD.at(column*2+1).toDouble())+column+1);
                }
                else
                {
                    columnsD[column].append((dataLineD.at(column*2+1).toDouble())+column);
                }
            }
        }
    } while (!lineD.isNull());
    fileD.close();

    //find largest and smallest values in y
    double yMin = columnsD[0].at(0);
    double yMax = columnsD[0].at(0);

    for (column = 0; column < nColumns; ++column)
    {
        for (int j = 0; j < columnsD[column].count(); j++)
        {
            if (columnsD[column].at(j) < yMin)
            {
                yMin = columnsD[column].at(j);
            }
            else
            if (columnsD[column].at(j) > yMax)
            {
                yMax = columnsD[column].at(j);
            }
        }
    }

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
    QVector< QVector<double> > xM;
    QVector< QVector<double> > columnsM;
    dataLineM.clear();
    xM.clear();
    columnsM.clear();
    xM.resize(nDataCol);
    columnsM.resize(nDataCol);
    lineM = streamM.readLine();
    do
    {
        lineM = streamM.readLine();
        dataLineM = lineM.split(" ", QString::SkipEmptyParts);
        if (dataLineM.count() == 0) break;
        for (column = 0; column < nColumns; ++column)
        {
            double yval = dataLineM.at(column*2+1).toDouble();
            if (yval != 0.0)
            {
                xM[column].append(dataLineM.at(0).toDouble());
                if (ui.plot2LogY->isChecked() == true)
                {
                    columnsM[column].append((dataLineM.at(column*2+1).toDouble())+column+1);
                }
                else
                {
                    columnsM[column].append((dataLineM.at(column*2+1).toDouble())+column);
                }
            }
        }
    } while (!lineM.isNull());
    fileM.close();

    //find largest and smallest values in x and check for y
    double xMin = xM[0].at(0);
    double xMax = xM[0].at(0);
    for (column = 0; column < nColumns; ++column)
    {
        for (int i = 0; i < xM[column].count(); i++)
        {
            if (xM[column].at(i) < xMin)
            {
                xMin = xM[column].at(i);
            }
            else
            if (xM[column].at(i) > xMax)
            {
                xMax = xM[column].at(i);
            }
        }
    }
    for (column = 0; column < nColumns; ++column)
    {
        for (int j = 0; j < columnsM[column].count(); j++)
        {
            if (columnsM[column].at(j) < yMin)
            {
                yMin = columnsM[column].at(j);
            }
            else
            if (columnsM[column].at(j) > yMax)
            {
                yMax = columnsM[column].at(j);
            }
        }
    }

    //open and read difference file to array
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
    QVector< QVector<double> > xDF;
    QVector< QVector<double> > columnsDF;
    dataLineDF.clear();
    xDF.clear();
    columnsDF.clear();
    xDF.resize(nDataCol);
    columnsDF.resize(nDataCol);
    lineDF = streamDF.readLine();
    do
    {
        lineDF = streamDF.readLine();
        dataLineDF = lineDF.split(" ", QString::SkipEmptyParts);
        if (dataLineDF.count() == 0) break;
        for (column = 0; column < nColumns; ++column)
        {
            double yval = dataLineDF.at(column*2+1).toDouble();
            if (yval != 0.0)
            {
                xDF[column].append(dataLineDF.at(0).toDouble());
                if (ui.plot2LogY->isChecked() == true)
                {
                    columnsDF[column].append((dataLineDF.at(column*2+1).toDouble())+column+1-0.2);
                }
                else
                {
                    columnsDF[column].append((dataLineDF.at(column*2+1).toDouble())+column-0.2);
                }
            }
        }
    } while (!lineDF.isNull());
    fileDF.close();

    //check for y
    for (column = 0; column < nColumns; ++column)
    {
        for (int j = 0; j < columnsDF[column].count(); j++)
        {
            if (columnsDF[column].at(j) < yMin)
            {
                yMin = columnsDF[column].at(j);
            }
            else
            if (columnsDF[column].at(j) > yMax)
            {
                yMax = columnsDF[column].at(j);
            }
        }
    }

    // create graph and assign data to it:
    QPen pen;
    QString datafileLabel;
    for (int i=0; i < nDatasets*3; i += 3)
    {
        ui.plot2->addGraph();
        pen.setColor(QColor(qSin(i*0.3)*100+100, qSin(i*0.6+0.7)*100+100, qSin(i*0.4+0.6)*100+100));
        ui.plot2->graph(i)->setPen(pen);
        ui.plot2->graph(i)->setData(xM.at(i/3), columnsM.at(i/3));
        ui.plot2->graph(i)->setName("&s model");
        ui.plot2->addGraph();
        ui.plot2->graph(i+1)->setPen(pen);
        ui.plot2->graph(i+1)->setData(xD.at(i/3), columnsD.at(i/3));
        ui.plot2->graph(i+1)->setLineStyle(QCPGraph::lsNone);
        ui.plot2->graph(i+1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 3));
        ui.plot2->graph(i+1)->setName("&s data");
        ui.plot2->addGraph();
        ui.plot2->graph(i+2)->setData(xDF.at(i/3), columnsDF.at(i/3));
        ui.plot2->graph(i+2)->setPen(QPen(Qt::gray));
        ui.plot2->graph(i+2)->setName("&s difference");
        QCPItemText *dataLabel = new QCPItemText(ui.plot2);
        ui.plot2->addItem(dataLabel);
        dataLabel->position->setCoords(xMax,(i/3)+0.2);
        dataLabel->setPositionAlignment(Qt::AlignRight);
        datafileLabel = dataFileList.at(i/3);
        dataLabel->setText(qPrintable(datafileLabel));
    }

    if (ui.plot2LogX->isChecked() == true)
    {
        ui.plot2->xAxis->setScaleType(QCPAxis::stLogarithmic);
        ui.plot2->xAxis->setScaleLogBase(100);
        ui.plot2->xAxis->setNumberFormat("eb");
        ui.plot2->xAxis->setNumberPrecision(0);
        ui.plot2->xAxis->setSubTickCount(9);
    }
    else
    {
        ui.plot2->xAxis->setScaleType(QCPAxis::stLinear);
        ui.plot2->xAxis->setNumberPrecision(9);
        ui.plot2->xAxis->setNumberFormat("gb");
    }
    if (ui.plot2LogY->isChecked() == true)
    {
        ui.plot2->yAxis->setScaleType(QCPAxis::stLogarithmic);
        ui.plot2->yAxis->setScaleLogBase(100);
        ui.plot2->yAxis->setNumberFormat("eb");
        ui.plot2->yAxis->setNumberPrecision(0);
        ui.plot2->yAxis->setSubTickCount(9);
    }
    else
    {
        ui.plot2->yAxis->setScaleType(QCPAxis::stLinear);
        ui.plot2->yAxis->setNumberPrecision(9);
        ui.plot2->yAxis->setNumberFormat("gb");
    }

    // give the axes some labels:
    ui.plot2->xAxis->setLabel("Q / Å\u207B\u00B9");
    ui.plot2->yAxis->setLabel("F(Q)");

    //plot
    ui.plot2->rescaleAxes();
    ui.plot2->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
    ui.plot2->xAxis->setRangeLower(xMin);
    ui.plot2->xAxis->setRangeUpper(xMax);
    ui.plot2->yAxis->setRangeLower(yMin);
    ui.plot2->yAxis->setRangeUpper(yMax+(yMax/10));
    ui.plot2->replot();
    return 0;
}

bool MainWindow::frplot2()
{
    //Filenames and number of datasets
    if (dataFileList.isEmpty()) return 0;
    int nDatasets = dataFileList.count();
    int nDataCol = nDatasets*2+1;

    baseFileName_= (workingDir_+epsrInpFileName_).split(".").at(0);
    QString frmodelFileName;
    frmodelFileName = (baseFileName_+".EPSR.x01");
    QString frdataFileName;
    frdataFileName = (baseFileName_+".EPSR.w01");
    QFile fileM(frmodelFileName);
    QFile fileD(frdataFileName);
    int column = 0;

    //open and read data file to array
    if(!fileD.open(QFile::ReadOnly | QFile::Text))
    {
        QMessageBox msgBox;
        msgBox.setText("Could not open .w01 file.");
        msgBox.exec();
        return 0;
    }
    QTextStream streamD(&fileD);
    QString lineD;
    QStringList dataLineD;
    QVector< QVector<double> >  xD;
    QVector< QVector<double> > columnsD;
    dataLineD.clear();
    xD.clear();
    columnsD.clear();
    xD.resize(nDataCol);
    columnsD.resize(nDataCol);
    int nColumns = 0;
    lineD = streamD.readLine();
    do
    {
        lineD = streamD.readLine();
        dataLineD = lineD.split(" ", QString::SkipEmptyParts);
        if (dataLineD.count() == 0) break;
        nColumns = (dataLineD.count() - 1) / 2;
        for (column = 0; column < nColumns; ++column)
        {
            double yval = dataLineD.at(column*2+1).toDouble();
            if (yval != 0.0)
            {
                xD[column].append(dataLineD.at(0).toDouble());
                if (ui.plot2LogY->isChecked() == true)
                {
                    columnsD[column].append((dataLineD.at(column*2+1).toDouble())+column+1);
                }
                else
                {
                    columnsD[column].append((dataLineD.at(column*2+1).toDouble())+column);
                }
            }
        }
    } while (!lineD.isNull());
    fileD.close();

    //find largest and smallest values in y
    double yMin = columnsD[0].at(0);
    double yMax = columnsD[0].at(0);
    for (column = 0; column < nColumns; ++column)
    {
        for (int j = 0; j < columnsD[column].count(); j++)
        {
            if (columnsD[column].at(j) < yMin)
            {
                yMin = columnsD[column].at(j);
            }
            else
            if (columnsD[column].at(j) > yMax)
            {
                yMax = columnsD[column].at(j);
            }
        }
    }

    //open and read model file to array
    if(!fileM.open(QFile::ReadOnly | QFile::Text))
    {
        QMessageBox msgBox;
        msgBox.setText("Could not open .x01 file.");
        msgBox.exec();
        return 0;
    }
    QTextStream streamM(&fileM);
    QString lineM;
    QStringList dataLineM;
    QVector< QVector<double> > xM;
    QVector< QVector<double> > columnsM;
    dataLineM.clear();
    xM.clear();
    columnsM.clear();
    xM.resize(nDataCol);
    columnsM.resize(nDataCol);
    lineM = streamM.readLine();
    do
    {
        lineM = streamM.readLine();
        dataLineM = lineM.split(" ", QString::SkipEmptyParts);
        if (dataLineM.count() == 0) break;
        for (column = 0; column < nColumns; ++column)
        {
            double yval = dataLineM.at(column*2+1).toDouble();
            if (yval != 0.0)
            {
                xM[column].append(dataLineM.at(0).toDouble());
                if (ui.plot2LogY->isChecked() == true)
                {
                    columnsM[column].append((dataLineM.at(column*2+1).toDouble())+column+1);
                }
                else
                {
                    columnsM[column].append((dataLineM.at(column*2+1).toDouble())+column);
                }
            }
        }
    } while (!lineM.isNull());
    fileM.close();

    //find largest and smallest values in x and check for y
    double xMin = xM[0].at(0);
    double xMax = xM[0].at(0);
    for (column = 0; column < nColumns; ++column)
    {
        for (int i = 0; i < xM[column].count(); i++)
        {
            if (xM[column].at(i) < xMin)
            {
                xMin = xM[column].at(i);
            }
            else
            if (xM[column].at(i) > xMax)
            {
                xMax = xM[column].at(i);
            }
        }
    }
    for (column = 0; column < nColumns; ++column)
    {
        for (int j = 0; j < columnsM[column].count(); j++)
        {
            if (columnsM[column].at(j) < yMin)
            {
                yMin = columnsM[column].at(j);
            }
            else
            if (columnsM[column].at(j) > yMax)
            {
                yMax = columnsM[column].at(j);
            }
        }
    }

    // create graph and assign data to it:
    QPen pen;
    QString datafileLabel;
    for (int i=0; i < nDatasets*2; i += 2)
    {
        ui.plot2->addGraph();
        pen.setColor(QColor(qSin(i*0.3)*100+100, qSin(i*0.6+0.7)*100+100, qSin(i*0.4+0.6)*100+100));
        ui.plot2->graph(i)->setPen(pen);
        ui.plot2->graph(i)->setData(xM.at(i/2), columnsM.at(i/2));
        ui.plot2->addGraph();
        ui.plot2->graph(i+1)->setPen(pen);
        ui.plot2->graph(i+1)->setData(xD.at(i/2), columnsD.at(i/2));
        ui.plot2->graph(i+1)->setLineStyle(QCPGraph::lsNone);
        ui.plot2->graph(i+1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 3));
        QCPItemText *dataLabel = new QCPItemText(ui.plot2);
        ui.plot2->addItem(dataLabel);
        dataLabel->position->setCoords(xMax,(i/2)+0.2);
        dataLabel->setPositionAlignment(Qt::AlignRight);
        datafileLabel = dataFileList.at(i/2);
        dataLabel->setText(qPrintable(datafileLabel));
    }

    if (ui.plot2LogX->isChecked() == true)
    {
        ui.plot2->xAxis->setScaleType(QCPAxis::stLogarithmic);
        ui.plot2->xAxis->setScaleLogBase(100);
        ui.plot2->xAxis->setNumberFormat("eb");
        ui.plot2->xAxis->setNumberPrecision(0);
        ui.plot2->xAxis->setSubTickCount(9);
    }
    else
    {
        ui.plot2->xAxis->setScaleType(QCPAxis::stLinear);
        ui.plot2->xAxis->setNumberPrecision(9);
        ui.plot2->xAxis->setNumberFormat("gb");
    }
    if (ui.plot2LogY->isChecked() == true)
    {
        ui.plot2->yAxis->setScaleType(QCPAxis::stLogarithmic);
        ui.plot2->yAxis->setScaleLogBase(100);
        ui.plot2->yAxis->setNumberFormat("eb");
        ui.plot2->yAxis->setNumberPrecision(0);
        ui.plot2->yAxis->setSubTickCount(9);
    }
    else
    {
        ui.plot2->yAxis->setScaleType(QCPAxis::stLinear);
        ui.plot2->yAxis->setNumberPrecision(9);
        ui.plot2->yAxis->setNumberFormat("gb");
    }

    // give the axes some labels:
    ui.plot2->xAxis->setLabel("r / Å");
    ui.plot2->yAxis->setLabel("G(r)");

    //plot
    ui.plot2->rescaleAxes();
    ui.plot2->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
    ui.plot2->xAxis->setRangeLower(xMin);
    ui.plot2->xAxis->setRangeUpper(xMax);
    ui.plot2->yAxis->setRangeLower(yMin);
    ui.plot2->yAxis->setRangeUpper(yMax+(yMax/10));
    ui.plot2->replot();
    return 0;
}

bool MainWindow::Eplot2()
{
    baseFileName_= (workingDir_+epsrInpFileName_).split(".").at(0);
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
    QVector<double> y;          //energy
    dataLine.clear();
    x.clear();
    y.clear();
    for (int iterations = 1; iterations < 1000000; ++iterations)
    {
        line = stream.readLine();
        dataLine = line.split(" ", QString::SkipEmptyParts);
        if (dataLine.count() == 0) break;
        y.append(dataLine.at(0).toDouble());
        x.append(iterations);
    }
    file.close();

    // create graph and assign data to it:
    ui.plot2->addGraph();
    ui.plot2->graph()->setData(x, y);
    if (ui.plot2LogX->isChecked() == true)
    {
        ui.plot2->xAxis->setScaleType(QCPAxis::stLogarithmic);
        ui.plot2->xAxis->setScaleLogBase(100);
        ui.plot2->xAxis->setNumberFormat("eb");
        ui.plot2->xAxis->setNumberPrecision(0);
        ui.plot2->xAxis->setSubTickCount(9);
    }
    else
    {
        ui.plot2->xAxis->setScaleType(QCPAxis::stLinear);
        ui.plot2->xAxis->setNumberPrecision(9);
        ui.plot2->xAxis->setNumberFormat("gb");
    }
    if (ui.plot2LogY->isChecked() == true)
    {
        ui.plot2->yAxis->setScaleType(QCPAxis::stLogarithmic);
        ui.plot2->yAxis->setScaleLogBase(100);
        ui.plot2->yAxis->setNumberFormat("eb");
        ui.plot2->yAxis->setNumberPrecision(0);
        ui.plot2->yAxis->setSubTickCount(9);
    }
    else
    {
        ui.plot2->yAxis->setScaleType(QCPAxis::stLinear);
        ui.plot2->yAxis->setNumberPrecision(9);
        ui.plot2->yAxis->setNumberFormat("gb");
    }

    // give the axes some labels:
    ui.plot2->xAxis->setLabel("iteration");
    ui.plot2->yAxis->setLabel("Energy");

    //plot
    ui.plot2->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
    ui.plot2->rescaleAxes();
    ui.plot2->replot();
    return 0;
}

bool MainWindow::Rplot2()
{
    baseFileName_= (workingDir_+epsrInpFileName_).split(".").at(0);
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
    QVector<double> y;          //R-factor
    dataLine.clear();
    x.clear();
    y.clear();
    for (int iterations = 1; iterations < 1000000; ++iterations)
    {
        line = stream.readLine();
        dataLine = line.split(" ", QString::SkipEmptyParts);
        if (dataLine.count() == 0) break;
        y.append(dataLine.at(2).toDouble());
        x.append(iterations);
    }
    file.close();

    // create graph and assign data to it:
    ui.plot2->addGraph();
    ui.plot2->graph()->setData(x, y);
    if (ui.plot2LogX->isChecked() == true)
    {
        ui.plot2->xAxis->setScaleType(QCPAxis::stLogarithmic);
        ui.plot2->xAxis->setScaleLogBase(100);
        ui.plot2->xAxis->setNumberFormat("eb");
        ui.plot2->xAxis->setNumberPrecision(0);
        ui.plot2->xAxis->setSubTickCount(9);
    }
    else
    {
        ui.plot2->xAxis->setScaleType(QCPAxis::stLinear);
        ui.plot2->xAxis->setNumberPrecision(9);
        ui.plot2->xAxis->setNumberFormat("gb");
    }
    if (ui.plot2LogY->isChecked() == true)
    {
        ui.plot2->yAxis->setScaleType(QCPAxis::stLogarithmic);
        ui.plot2->yAxis->setScaleLogBase(100);
        ui.plot2->yAxis->setNumberFormat("eb");
        ui.plot2->yAxis->setNumberPrecision(0);
        ui.plot2->yAxis->setSubTickCount(9);
    }
    else
    {
        ui.plot2->yAxis->setScaleType(QCPAxis::stLinear);
        ui.plot2->yAxis->setNumberPrecision(9);
        ui.plot2->yAxis->setNumberFormat("gb");
    }

    // give the axes some labels:
    ui.plot2->xAxis->setLabel("iteration");
    ui.plot2->yAxis->setLabel("R-factor");

    //plot
    ui.plot2->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
    ui.plot2->rescaleAxes();
    ui.plot2->replot();
    return 0;
}

bool MainWindow::Pplot2()
{
    baseFileName_= (workingDir_+epsrInpFileName_).split(".").at(0);
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
    ui.plot2->addGraph();
    ui.plot2->graph()->setData(x, y);
    if (ui.plot2LogX->isChecked() == true)
    {
        ui.plot2->xAxis->setScaleType(QCPAxis::stLogarithmic);
        ui.plot2->xAxis->setScaleLogBase(100);
        ui.plot2->xAxis->setNumberFormat("eb");
        ui.plot2->xAxis->setNumberPrecision(0);
        ui.plot2->xAxis->setSubTickCount(9);
    }
    else
    {
        ui.plot2->xAxis->setScaleType(QCPAxis::stLinear);
        ui.plot2->xAxis->setNumberPrecision(9);
        ui.plot2->xAxis->setNumberFormat("gb");
    }
    if (ui.plot2LogY->isChecked() == true)
    {
        ui.plot2->yAxis->setScaleType(QCPAxis::stLogarithmic);
        ui.plot2->yAxis->setScaleLogBase(100);
        ui.plot2->yAxis->setNumberFormat("eb");
        ui.plot2->yAxis->setNumberPrecision(0);
        ui.plot2->yAxis->setSubTickCount(9);
    }
    else
    {
        ui.plot2->yAxis->setScaleType(QCPAxis::stLinear);
        ui.plot2->yAxis->setNumberPrecision(9);
        ui.plot2->yAxis->setNumberFormat("gb");
    }

    // give the axes some labels:
    ui.plot2->xAxis->setLabel("iteration");
    ui.plot2->yAxis->setLabel("Pressure / kbar");

    //plot
    ui.plot2->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
    ui.plot2->rescaleAxes();
    ui.plot2->replot();
    return 0;
}

bool MainWindow::Ereqplot2()
{
    baseFileName_= (workingDir_+epsrInpFileName_).split(".").at(0);
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
    QVector<double> x2;          //ereq
    QVector<double> y1;         //fit quality
    QVector<double> y2;         //fitted line showing gradient of last 50 iterations
    dataLineerg.clear();
    x1.clear();
    x2.clear();
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
            msgBox.setText("This plot type is not compatible with the version of EPSR this simulation was last run with.");
            msgBox.exec();
            fileqdr.close();
            return 0;
        }
        x2.append(dataLineqdr.at(3).toDouble());
        y2.append(dataLineqdr.at(4).toDouble());
    }
    fileqdr.close();

    // create graph and assign data to it:
    QCPCurve *ergCurve = new QCPCurve(ui.plot2->xAxis, ui.plot2->yAxis);
    ui.plot2->addPlottable(ergCurve);
    ergCurve->setData(x1, y1);
    ui.plot2->addGraph();
    ui.plot2->graph(0)->setData(x2, y2);
    ui.plot2->graph(0)->setLineStyle(QCPGraph::lsNone);
    ui.plot2->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 3));

    ui.plot2->xAxis->setScaleType(QCPAxis::stLinear);
    ui.plot2->xAxis->setNumberPrecision(9);
    ui.plot2->xAxis->setNumberFormat("gb");
    ui.plot2->yAxis->setScaleType(QCPAxis::stLinear);
    ui.plot2->yAxis->setNumberPrecision(9);
    ui.plot2->yAxis->setNumberFormat("gb");

    // give the axes some labels:
    ui.plot2->xAxis->setLabel("ereq energy");
    ui.plot2->yAxis->setLabel("quality of fit");

    //plot
    ui.plot2->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
    ui.plot2->rescaleAxes();
    ui.plot2->replot();
    return 0;
}

void MainWindow::showPointToolTip2(QMouseEvent *event)
{
    double x2 = this->ui.plot2->xAxis->pixelToCoord(event->pos().x());
    double y2 = this->ui.plot2->yAxis->pixelToCoord(event->pos().y());

//    setToolTip(QString("%1 , %2").arg(x).arg(y));

    QString x2str;
    x2str.sprintf(" %5g", x2);
    QString y2str;
    y2str.sprintf(" %5g", y2);
    QString value2 = x2str+","+y2str;
    ui.mousecoord2->setText(value2);
}

void MainWindow::plotZoom2(QWheelEvent* event)
{
    if (event->modifiers() == Qt::ShiftModifier)
    {
        ui.plot2->axisRect()->setRangeZoomAxes(ui.plot2->xAxis,ui.plot2->xAxis);
    }
    else if (event->modifiers() == Qt::ControlModifier)
    {
        ui.plot2->axisRect()->setRangeZoomAxes(ui.plot2->yAxis,ui.plot2->yAxis);
    }
    else
    {
        ui.plot2->axisRect()->setRangeZoomAxes(ui.plot2->xAxis,ui.plot2->yAxis);
    }
}
