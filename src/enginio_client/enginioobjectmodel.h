/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://qt.digia.com/contact-us
**
** This file is part of the Enginio Qt Client Library.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

#ifndef ENGINIOOBJECTMODEL_H
#define ENGINIOOBJECTMODEL_H

#include "enginioclient_global.h"
#include <QAbstractListModel>

class EnginioAbstractObject;
class EnginioObjectModelPrivate;

namespace Enginio {
    enum ObjectModelRole {
        DataRole = Qt::UserRole + 1,
        JsonRole
    };
}

class ENGINIOCLIENT_EXPORT EnginioObjectModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int rowCount READ rowCount NOTIFY rowCountChanged)

public:
    explicit EnginioObjectModel(QObject *parent = 0);
    ~EnginioObjectModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QModelIndex index(int row, int column = 0,
                      const QModelIndex & parent = QModelIndex()) const;
    QVariant data(int row, int role = Qt::DisplayRole) const;
    QVariant data(const QModelIndex &index,
                  int role = Qt::DisplayRole) const;
    bool setData (const QModelIndex &index,
                  const QVariant &value,
                  int role = Qt::EditRole);

    bool addToModel(QList<EnginioAbstractObject*> objects, int row);
    bool removeFromModel(int row, int count);
    Q_INVOKABLE bool clear();
    QModelIndex indexFromId(const QString &id) const;
    EnginioAbstractObject * getObject(const QModelIndex &index) const;
    void notifyObjectUpdated(const QString &id);

signals:
    void rowCountChanged(int newCount) const;

protected:
    EnginioObjectModelPrivate *d_ptr;
    EnginioObjectModel(EnginioObjectModelPrivate &dd,
                       QObject *parent = 0);

private:
    Q_DECLARE_PRIVATE(EnginioObjectModel)
    Q_DISABLE_COPY(EnginioObjectModel)
};

#endif // ENGINIOOBJECTMODEL_H
