/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "diagramitem.h"
#include "arrow.h"

#include "properties-view.h"
#include <QGraphicsScene>
#include <QGraphicsSceneContextMenuEvent>
#include <QMenu>
#include <QPainter>
#include <olsapp.h>

//! [0]
DiagramItem::DiagramItem(DiagramType diagramType, QMenu* contextMenu,
    QGraphicsItem* parent)
    : QGraphicsPolygonItem(parent)
{
    myDiagramType = diagramType;
    myContextMenu = contextMenu;

    QPainterPath path;
    switch (myDiagramType) {
    case StartEnd:
        path.moveTo(200, 50);
        path.arcTo(150, 0, 50, 50, 0, 90);
        path.arcTo(50, 0, 50, 50, 90, 90);
        path.arcTo(50, 50, 50, 50, 180, 90);
        path.arcTo(150, 50, 50, 50, 270, 90);
        path.lineTo(200, 25);
        myPolygon = path.toFillPolygon();
        break;
    case Conditional:
        myPolygon << QPointF(-100, 0) << QPointF(0, 100)
                  << QPointF(100, 0) << QPointF(0, -100)
                  << QPointF(-100, 0);
        break;
    case Step:
        myPolygon << QPointF(-100, -100) << QPointF(100, -100)
                  << QPointF(100, 100) << QPointF(-100, 100)
                  << QPointF(-100, -100);
        break;
    default:
        myPolygon << QPointF(-120, -80) << QPointF(-70, 80)
                  << QPointF(120, 80) << QPointF(70, -80)
                  << QPointF(-120, -80);
        break;
    }
    setPolygon(myPolygon);
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
}
//! [0]

//! [1]
void DiagramItem::removeArrow(Arrow* arrow)
{
    int index = arrows.indexOf(arrow);

    if (index != -1)
        arrows.removeAt(index);
}
//! [1]

//! [2]
void DiagramItem::removeArrows()
{
    foreach (Arrow* arrow, arrows) {
        arrow->startItem()->removeArrow(arrow);
        arrow->endItem()->removeArrow(arrow);
        scene()->removeItem(arrow);
        delete arrow;
    }
}
//! [2]

//! [3]
void DiagramItem::addArrow(Arrow* arrow)
{
    arrows.append(arrow);
}
//! [3]

//! [4]
QPixmap DiagramItem::image() const
{
    QPixmap pixmap(250, 250);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setPen(QPen(Qt::black, 8));
    painter.translate(125, 125);
    painter.drawPolyline(myPolygon);

    return pixmap;
}
//! [4]

//! [5]
void DiagramItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
    scene()->clearSelection();
    setSelected(true);
    myContextMenu->exec(event->screenPos());
}
//! [5]

//! [6]
QVariant DiagramItem::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == QGraphicsItem::ItemPositionChange) {
        foreach (Arrow* arrow, arrows) {
            arrow->updatePosition();
        }
    }

    return value;
}
//! [6]

ols_properties_t* get_properties(const char* id)
{
    ols_properties_t* props;
    ols_property_t* devices;
    ols_property_t* rate;

    props = ols_properties_create();

    devices = ols_properties_add_list(props, "device_id",
        ("Device"),
        OLS_COMBO_TYPE_LIST,
        OLS_COMBO_FORMAT_STRING);

    ols_property_list_add_string(devices, "Default", "default");

    ols_properties_add_text(props, "custom_pcm", ("PCM"),
        OLS_TEXT_DEFAULT);

    rate = ols_properties_add_list(props, "rate", ("Rate"),
        OLS_COMBO_TYPE_LIST,
        OLS_COMBO_FORMAT_INT);

    //obs_property_set_modified_callback(devices, alsa_devices_changed);

    ols_property_list_add_int(rate, "32000 Hz", 32000);
    ols_property_list_add_int(rate, "44100 Hz", 44100);
    ols_property_list_add_int(rate, "48000 Hz", 48000);

    ols_property_list_add_string(devices, "Custom", "__custom__");
    return props;
}
void DiagramItem::editProperties()
{

    ols_data_t* settings = ols_data_create();

    ols_data_set_default_int(settings, "color", 0xFFFFFFFF);
    ols_data_set_default_int(settings, "width", 400);
    ols_data_set_default_int(settings, "height", 400);

    propertiesView = new OLSPropertiesView(
        settings, "decklink_output",
        (PropertiesReloadCallback)get_properties, 170);
    propertiesView->show();
    //  ui->propertiesLayout->addWidget(propertiesView);
    //ols_data_release(settings);

    //    connect(propertiesView, SIGNAL(Changed()), this,
    //        SLOT(PropertiesChanged()));
}
