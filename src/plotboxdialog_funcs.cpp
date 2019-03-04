#include "plotboxdialog.h"
#include "epsrproject.h"

#include <QPushButton>

PlotBoxDialog::PlotBoxDialog(MainWindow *parent) : QDialog(parent)
{
    ui.setupUi(this);

    mainWindow_ = parent;
    atoAtomTypes_.clear();
    atoAtomTypes_ = mainWindow_->atomTypes();
    workingDir_ = mainWindow_->workingDir();
    atoFileName_ = mainWindow_->atoFileName();
    epsrBinDir_ = mainWindow_->epsrBinDir();

    QRegExp noNegIntrx("^\\d*$");
    QRegExp onlyDecrx("^\\d*\\.?\\d*$");    
    QRegExp possibleNegDecrx("^-?\\d*\\.?\\d*$");
    QRegExp threeIntrx("^\\d*\\ \\d*\\ \\d*$");
    ui.plotAtoCentreLineEdit->setValidator((new QRegExpValidator(noNegIntrx, this)));
    ui.plotAtoMaxXLineEdit->setValidator((new QRegExpValidator(onlyDecrx, this)));
    ui.plotAtoMaxYLineEdit->setValidator((new QRegExpValidator(onlyDecrx, this)));
    ui.plotAtoMaxZLineEdit->setValidator((new QRegExpValidator(onlyDecrx, this)));
    ui.plotAtoMinZLineEdit->setValidator((new QRegExpValidator(possibleNegDecrx, this)));
    ui.plotAtoRotLineEdit->setValidator((new QRegExpValidator(threeIntrx, this)));

    ui.atoAtomList->clear();
    for (int n=0; n < atoAtomTypes_.count(); ++n)
    {
        QListWidgetItem* item = new QListWidgetItem(atoAtomTypes_.at(n));
        item->setData(Qt::UserRole, n);
        ui.atoAtomList->addItem(item);
    }
}

void PlotBoxDialog::on_plotBoxButton_clicked(bool checked)
{
    if (ui.plotAtoCentreLineEdit->text().isEmpty())
    {
        QMessageBox msgBox;
        msgBox.setText("One of the parameters required for plotting is missing");
        msgBox.exec();
        return;
    }

    QString maxDist;
    QString rotCoord;

    if (ui.plotAtoCentreLineEdit->text() != "0")
    {
        if (ui.plotAtoMaxXLineEdit->text().isEmpty() || ui.plotAtoMaxYLineEdit->text().isEmpty()
            || ui.plotAtoMaxZLineEdit->text().isEmpty() || ui.plotAtoMinZLineEdit->text().isEmpty() || ui.plotAtoRotLineEdit->text().isEmpty()
            || ui.plotAtoRotLineEdit->text().split(" ", QString::SkipEmptyParts).count() != 3)
        {
            QMessageBox msgBox;
            msgBox.setText("One of the parameters required for plotting is missing");
            msgBox.exec();
            return;
        }
        maxDist = ui.plotAtoMaxXLineEdit->text()+" "+ui.plotAtoMaxYLineEdit->text()+" "+ui.plotAtoMinZLineEdit->text()+" "+ui.plotAtoMaxZLineEdit->text();
        rotCoord = ui.plotAtoRotLineEdit->text();
    }

    QString plotCentre = ui.plotAtoCentreLineEdit->text();

    QString listExcAtoms;
    QList<QListWidgetItem*> selectedItems = ui.atoAtomList->selectedItems();
    if (selectedItems.count() != 0)
    {
        foreach(QListWidgetItem* item, selectedItems)
        {
            listExcAtoms.append(QString::number((item->data(Qt::UserRole).toInt())+1)+" ");
        }
    }
    else
    {
        listExcAtoms = "0";
    }

    QString atoBaseFileName = atoFileName_.split(".",QString::SkipEmptyParts).at(0);
#ifdef _WIN32
    QFile jmolFile(workingDir_+"plot"+atoBaseFileName+".bat");
#else
    QFile jmolFile(workingDir_+"plot"+atoBaseFileName+".sh");
#endif
    if(!jmolFile.open(QFile::WriteOnly | QFile::Text))
    {
        QMessageBox msgBox;
        msgBox.setText("Could not make Jmol plotting file.");
        msgBox.exec();;
    }
#ifdef _WIN32
    if (plotCentre == "0")
    {
        QTextStream stream(&jmolFile);
        stream << "set EPSRbin=" << epsrBinDir_ << "\n"
               << "set EPSRrun=" << workingDir_ << "\n"
               << "%EPSRbin%plotato.exe %EPSRrun% plotato " << atoBaseFileName << " 3" << " "+plotCentre << " "+listExcAtoms << "\n";
        jmolFile.close();
    }
    else
    {
        QTextStream stream(&jmolFile);
        stream << "set EPSRbin=" << epsrBinDir_ << "\n"
               << "set EPSRrun=" << workingDir_ << "\n"
               << "%EPSRbin%plotato.exe %EPSRrun% plotato " << atoBaseFileName << " 3" << " "+plotCentre << " "+maxDist << " "+rotCoord << " "+listExcAtoms << "\n";
        jmolFile.close();
    }
#else
    if (plotCentre == "0")
    {
        QTextStream stream(&jmolFile);
        stream << "export EPSRbin=" << epsrBinDir_ << "\n"
               << "export EPSRrun=" << workingDir_ << "\n"
               << "\"$EPSRbin\"'plotato' \"$EPSRrun\" plotato " << atoBaseFileName << " 3" << " "+plotCentre << " "+listExcAtoms << "\n";
        jmolFile.close();
    }
    else
    {
        QTextStream stream(&jmolFile);
        stream << "export EPSRbin=" << epsrBinDir_ << "\n"
               << "export EPSRrun=" << workingDir_ << "\n"
               << "\"$EPSRbin\"'plotato' \"$EPSRrun\" plotato " << atoBaseFileName << " 3" << " "+plotCentre << " "+maxDist << " "+rotCoord << " "+listExcAtoms << "\n";
        jmolFile.close();
    }
#endif

    QDir::setCurrent(workingDir_);

    QProcess processplotato;
    processplotato.setProcessChannelMode(QProcess::ForwardedChannels);
#ifdef _WIN32
    processplotato.startDetached("plot"+atoBaseFileName+".bat");
#else
    processplotato.startDetached("sh plot"+atoBaseFileName+".sh");
#endif
//    ui.messagesLineEdit->setText("Box .ato file plotted in separate window");
}
