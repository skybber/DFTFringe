/******************************************************************************
**
**  Copyright 2016 Dale Eason
**  This file is part of DFTFringe
**  is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation version 3 of the License

** DFTFringe is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with DFTFringe.  If not, see <http://www.gnu.org/licenses/>.

****************************************************************************/
#include "settingsigram.h"
#include "ui_settingsigram.h"
#include <QSettings>
#include <QColorDialog>
#include <QPen>
#include <QPainter>
#include <QMenu>
static inline QString colorButtonStyleSheet(const QColor &bgColor)
{
    if (bgColor.isValid()) {
        QString rc = QLatin1String("border: 2px solid black; border-radius: 2px; background:");
        rc += bgColor.name();
        return rc;
    }
    return QLatin1String("border: 2px dotted black; border-radius: 2px;");
}
settingsIGram::settingsIGram(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::settingsIGram)
{
    ui->setupUi(this);
    QSettings set;
    edgeColor = QColor(set.value("igramEdgeLineColor", "white").toString());
    ui->edgeLineColorPb->setStyleSheet(colorButtonStyleSheet(edgeColor));
    centerColor = QColor(set.value("igramCenterLineColor", "yellow").toString());
    ui->centerLineColorPb->setStyleSheet(colorButtonStyleSheet(centerColor));
    ui->opacitySB->setValue(set.value("igramLineOpacity", 65.).toDouble());
    edgeWidth = set.value("igramEdgeWidth",3).toInt();
    centerWidth = set.value("igramCenterWidth",3).toInt();
    ui->spinBox->setValue(edgeWidth);
    ui->centerSpinBox->setValue(centerWidth);
    ui->zoomBoxWidthSb->setValue(set.value("igramZoomBoxWidth", 200).toDouble());

    connect(ui->buttonBox->button(QDialogButtonBox::Apply), SIGNAL(clicked()), SLOT(on_buttonBox_accepted()));
    ui->styleCB->setEditable(false);
    ui->styleCB->setIconSize(QSize(80,14));
    ui->styleCB->setMinimumWidth(80);

    for (int aaa = Qt::SolidLine; aaa < Qt::CustomDashLine; aaa++)
    {
        QPixmap pix(80,14);
        pix.fill(Qt::white);

        QBrush brush(Qt::black);
        QPen pen(Qt::black ,2.5,(Qt::PenStyle)aaa);

        QPainter painter(&pix);
        painter.setPen(pen);
        painter.drawLine(2,7,78,7);

        ui->styleCB->addItem(QIcon(pix),"");
    }
    int style = set.value("igramLineStyle", 1).toInt();
    ui->styleCB->setCurrentIndex(style-1);
    m_removeDistortion = set.value("removeLensDistortion", false).toBool();
    ui->removeDistortion->setChecked(m_removeDistortion);
    lensesModel = new LenseTableModel(this);
    QStringList lenses = set.value("Lenses", "").toString().split("\n");
    m_lenseParms = set.value("currentLense","").toString().split(",");

    int currentLensNdx = -1;
    m_lensData.clear();
    int ndx = 0;
    foreach(QString l, lenses){
        if (l == "")
            continue;
        m_lensData.push_back(l.split(","));
        if (m_lensData.back()[0] == m_lenseParms[0])
            currentLensNdx = ndx;
        ++ndx;
    }
    lensesModel->setLensData(&m_lensData);
    ui->lenseTableView->setModel(lensesModel);
    m_lenseParms = set.value("currentLense","").toString().split(",");
    if (!m_removeDistortion)
        ui->lenseTableView->hide();
    else {
        ui->currentlense->setText(m_lenseParms[0]);
    }
    ui->lenseTableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->lenseTableView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
    ui->lenseTableView->selectRow(currentLensNdx);
    ui->lenseTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    selectionModel = ui->lenseTableView->selectionModel();

    QPalette* palette = new QPalette();
    palette->setColor(QPalette::Highlight,"lightCyan");
    palette->setColor(QPalette::HighlightedText,"black");
    ui->lenseTableView->setPalette(*palette);
}

settingsIGram::~settingsIGram()
{
    delete ui;
}


void settingsIGram::on_edgeLineColorPb_clicked()
{
    edgeColor = QColorDialog::getColor(edgeColor, 0);
    ui->edgeLineColorPb->setStyleSheet(colorButtonStyleSheet(edgeColor));
}

void settingsIGram::on_spinBox_valueChanged(int arg1)
{
    edgeWidth = arg1;
}

void settingsIGram::on_centerSpinBox_valueChanged(int arg1)
{
    centerWidth = arg1;
}

void settingsIGram::on_centerLineColorPb_clicked()
{
    centerColor = QColorDialog::getColor(centerColor, 0);
    ui->centerLineColorPb->setStyleSheet(colorButtonStyleSheet(centerColor));
}

void settingsIGram::on_buttonBox_accepted()
{
    QSettings set;
    set.setValue("igramCenterLineColor", centerColor.name());
    set.setValue("igramEdgeLineColor", edgeColor.name());
    set.setValue("igramEdgeWidth", edgeWidth);
    set.setValue("igramCenterWidth", centerWidth);
    set.setValue("igramLineOpacity", ui->opacitySB->value());
    set.setValue("igramLineStyle", ui->styleCB->currentIndex() + 1);
    set.setValue("igramZoomBoxWidth", ui->zoomBoxWidthSb->value());
    emit igramLinesChanged(edgeWidth,centerWidth, edgeColor, centerColor, ui->opacitySB->value(),
                           ui->styleCB->currentIndex()+1, ui->zoomBoxWidthSb->value());
}

void settingsIGram::on_removeDistortion_clicked(bool checked)
{
    m_removeDistortion = checked;
    QSettings set;
    set.setValue("removeLensDistortion", checked);
    if (checked){
        ui->lenseTableView->show();
        QSettings set;

    }
    else {
        ui->lenseTableView->hide();
    }
}


#include <QDebug>

void settingsIGram::eraseItem()
{
    qDebug() << "erasing" << currentNdx;
    lensesModel->removeRow(currentNdx.row());
    saveLensData();
}
void settingsIGram::showContextMenu(const QPoint &pos)
{
    // Handle global position
    QPoint globalPos = ui->lenseTableView->mapToGlobal(pos);

    // Create menu and insert some actions
    QMenu myMenu;
    myMenu.addAction("Erase",  this, SLOT(eraseItem()));

    // Show context menu at handling position
    myMenu.exec(globalPos);
}
void settingsIGram::saveLensData(){
    QSettings set;
    QStringList v;
    foreach(QStringList l, m_lensData){
        v.push_back(l.join(","));
    }
    set.setValue("Lenses", v.join("\n"));
}

void settingsIGram::updateLenses(QString str){
    m_lensData.push_back(str.split(","));
    saveLensData();
    lensesModel->insertRow(m_lensData.size() -2);

}

void settingsIGram::on_lenseTableView_clicked(const QModelIndex &index)
{
    currentNdx = index;
    const QAbstractItemModel * model = index.model();
    ui->currentlense->setText(model->data(model->index(index.row(), 0, index.parent()), Qt::DisplayRole).toString());
    m_lenseParms = m_lensData[index.row()];
    lensesModel->setCurrentRow(index.row());

    ui->lenseTableView->selectionModel()->select( index,
                      QItemSelectionModel::ClearAndSelect|QItemSelectionModel::Rows);
    QSettings set;
    set.setValue("currentLense", m_lensData[index.row()].join(","));
}

