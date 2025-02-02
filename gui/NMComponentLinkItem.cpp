 /****************************************************************************** 
 * Created by Alexander Herzig 
 * Copyright 2010,2011,2012 Landcare Research New Zealand Ltd 
 *
 * This file is part of 'LUMASS', which is free software: you can redistribute
 * it and/or modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the License, 
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/
/*
 * NMComponentLinkItem.cpp
 *
 *  Created on: 20/06/2012
 *      Author: alex
 */

#include "nmlog.h"
#include "NMComponentLinkItem.h"
#include "math.h"
#include <QDebug>
#include <QTime>
#include "NMAggregateComponentItem.h"

NMComponentLinkItem::NMComponentLinkItem(NMProcessComponentItem* sourceItem,
		NMProcessComponentItem* targetItem,
		QGraphicsItem* parent)
    : QGraphicsPathItem(parent), mIsDynamic(false)
{
	this->mSourceItem = sourceItem;
	this->mTargetItem = targetItem;
	this->mHeadSize = 10;

    //	this->setFlag(QGraphicsItem::ItemIsSelectable, false);
}

NMComponentLinkItem::~NMComponentLinkItem()
{
}

void NMComponentLinkItem::setSourceItem(const NMProcessComponentItem* sourceItem)
{
	if (sourceItem != 0)
		this->mSourceItem = const_cast<NMProcessComponentItem*>(sourceItem);
}

void NMComponentLinkItem::setTargetItem(const NMProcessComponentItem* targetItem)
{
	if (targetItem != 0)
		this->mTargetItem = const_cast<NMProcessComponentItem*>(targetItem);
}

void
NMComponentLinkItem::setIsDynamic(bool dynamic)
{
    if (this->mIsDynamic != dynamic)
    {
        this->mIsDynamic = dynamic;
        this->update();
    }
}

QRectF
NMComponentLinkItem::boundingRect(void) const
{
    // determine coordinates and bounding rectangle
    QRectF srcBnd, tarBnd;

    NMAggregateComponentItem* eldestCollapsedTarget = 0;
    NMAggregateComponentItem* th = mTargetItem->getModelParent();
    if (th)
    {
        eldestCollapsedTarget = th->isCollapsed() ? th : 0;
        th->getEldestCollapsedAncestor(eldestCollapsedTarget);

        if (eldestCollapsedTarget)
        {
            tarBnd = mapRectFromItem(eldestCollapsedTarget, eldestCollapsedTarget->iconRect());
        }
        else
        {
            tarBnd = mapRectFromItem(mTargetItem, mTargetItem->boundingRect());
        }
    }
    else
    {
        tarBnd = mapRectFromItem(mTargetItem, mTargetItem->boundingRect());
    }

    NMAggregateComponentItem* eldestCollapsedSource = 0;
    NMAggregateComponentItem* ts = mSourceItem->getModelParent();
    if (ts)
    {
        eldestCollapsedSource = ts->isCollapsed() ? ts : 0;
        ts->getEldestCollapsedAncestor(eldestCollapsedSource);

        if (eldestCollapsedSource)
        {
            srcBnd = mapRectFromItem(eldestCollapsedSource, eldestCollapsedSource->iconRect());
        }
        else
        {
            srcBnd = mapRectFromItem(mSourceItem, mSourceItem->boundingRect());
        }
    }
    else
    {
        srcBnd = mapRectFromItem(mSourceItem, mSourceItem->boundingRect());
    }

    return srcBnd.united(tarBnd);
}

void
NMComponentLinkItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
		QWidget* widget)
{
    if (mSourceItem->collidesWithItem(mTargetItem))
    {
        return;
    }

    // ----------------------------------------------------------
    //                  some variables
    // ----------------------------------------------------------
    QPointF sp;
    QPointF tp;
    QLineF polyEdge;
    QPointF p1, p2;

    QPointF ip;
    QPolygonF poly;
    QPointF sip;
    QPolygonF spoly;

    // ----------------------------------------------------------
    //                  determine target shape
    // ----------------------------------------------------------
    NMAggregateComponentItem* eldestCollapsedTarget = 0;
    NMAggregateComponentItem* th = mTargetItem->getModelParent();
    if (th)
    {
        eldestCollapsedTarget = th->isCollapsed() ? th : 0;
        th->getEldestCollapsedAncestor(eldestCollapsedTarget);
    }

    if (eldestCollapsedTarget)
    {
        poly = QPolygonF(mapRectFromItem(eldestCollapsedTarget,
                                         eldestCollapsedTarget->iconRect()));
        tp = mapFromItem(eldestCollapsedTarget, 0, 0);
    }
    else
    {
        poly = QPolygonF(mapFromItem(mTargetItem, mTargetItem->getShapeAsPolygon()));
        tp = mapFromItem(mTargetItem, 0, 0);
    }

    // ----------------------------------------------------------
    //                  determine source shape
    // ----------------------------------------------------------
    //QGraphicsItem* sourceHost = mSourceItem->parentItem();
    NMAggregateComponentItem* eldestCollapsedSource = 0;
    NMAggregateComponentItem* ts = mSourceItem->getModelParent();
    if (ts)
    {
        eldestCollapsedSource = ts->isCollapsed() ? ts : 0;
        ts->getEldestCollapsedAncestor(eldestCollapsedSource);
    }

    if (eldestCollapsedSource)
    {
        spoly = QPolygonF(mapRectFromItem(eldestCollapsedSource,
                                          eldestCollapsedSource->iconRect()));
        sp = mapFromItem(eldestCollapsedSource, 0, 0);
    }
    else
    {
        spoly = QPolygonF(mapFromItem(mSourceItem, mSourceItem->getShapeAsPolygon()));
        sp = mapFromItem(mSourceItem, 0, 0);
    }


    // check if link is between components inside the same
    // collapsed component, we don't have to draw anything
    if (    eldestCollapsedSource
        &&  eldestCollapsedTarget
        &&  (   eldestCollapsedSource
             == eldestCollapsedTarget
            )
       )
    {
        return;
    }

    // ----------------------------------------------------------
    //      determine source & target intersection points
    // ----------------------------------------------------------
    QLineF linkLine(sp, tp);

    // target intersection point
    for (unsigned int i=0; i < poly.count()-1; ++i)
    {
        p1 = poly.at(i);
        p2 = poly.at(i+1);
        polyEdge = QLineF(p1, p2);
        if (polyEdge.intersects(linkLine, &ip) == QLineF::BoundedIntersection)
                break;
    }

	// determine source intersection point
    for (unsigned int i=0; i < spoly.count()-1; ++i)
    {
        p1 = spoly.at(i);
        p2 = spoly.at(i+1);
        polyEdge = QLineF(p1, p2);
        if (polyEdge.intersects(linkLine, &sip) == QLineF::BoundedIntersection)
                break;
    }

    // ----------------------------------------------------------
    //              draw link
    // ----------------------------------------------------------
	QLineF shortBase(ip, sp);
	shortBase.setLength(mHeadSize);
	QLineF shortBase2(shortBase.p2(), shortBase.p1());
	QLineF normal1 = shortBase2.normalVector();
	normal1.setLength(mHeadSize / 2.0);
	QLineF normal2(normal1.p2(), normal1.p1());
	normal2.setLength(mHeadSize);

	QPolygonF head;
	head << ip << normal2.p1() << normal2.p2();

	// create bezier curve
	QPointF ep(shortBase.p2());
	QPainterPath path(sip);
    path.lineTo(ep);

	// check, whether any component item is colliding with
	// the direct line
	//QList<QGraphicsItem*> closeItems = this->collidingItems(Qt::IntersectsItemBoundingRect);
    //
	//foreach(const QGraphicsItem* i, closeItems)
	//{
	//	if (i->type() == NMProcessComponentItem::Type    ||
	//		i->type() == NMAggregateComponentItem::Type)
	//	{
	//		if (i->collidesWithPath(path, Qt::IntersectsItemShape))
	//		{
	//			NMDebug(<< QTime::currentTime().msec() << ": line hits!" << std::endl);
	//		}
	//	}
	//}


    //QPointF cp1(ep.x(), (ep.y() - sip.y()) * 0.33 + sip.y());
    //QPointF cp2(sip.x(), (ep.y() - sip.y()) * 0.67 + sip.y());
    //path.cubicTo(cp1, cp2, ep);

	// draw elements
	QPen pen;
    if (this->mIsDynamic)
        pen = QPen(QBrush(QColor(80,80,80)), 1.8, Qt::DashLine);
    else
        pen = QPen(QBrush(QColor(80,80,80)), 1.8, Qt::SolidLine);
	painter->setPen(pen);
	painter->drawPath(path);

    if (this->mIsDynamic)
    {
        pen = QPen(QBrush(QColor(80,80,80)), 1.8, Qt::SolidLine);
        painter->setPen(pen);
        painter->setBrush(QColor(255,255,255));
        painter->drawPolygon(head);
    }
    else
    {
        pen = QPen(QBrush(QColor(80,80,80)), 1.8, Qt::SolidLine);
        painter->setPen(pen);
        painter->setBrush(QColor(80,80,80));
        painter->drawPolygon(head);
    }

	QPainterPath npath = path;
	npath.addPolygon(head);
	this->mPath = npath;
	setPath(mPath);
}

QDataStream& operator<<(QDataStream &data, const NMComponentLinkItem &item)
{
	NMComponentLinkItem& i = const_cast<NMComponentLinkItem&>(item);
	//data << (qint32)NMComponentLinkItem::Type;

	data << (qint32)i.sourceItem()->getOutputLinkIndex(i.sourceItem()->getTitle());
	data << i.sourceItem()->getTitle();

	data << (qint32)i.targetItem()->getInputLinkIndex(i.targetItem()->getTitle());
	data << i.targetItem()->getTitle();

    data << i.getIsDynamic();
    //data << i.isVisible();

	return data;
}

