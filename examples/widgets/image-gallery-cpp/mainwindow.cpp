#include <QtWidgets>
#include <QDebug>
#include <QFrame>
#include <QLabel>
#include <QListView>
#include <QMenu>
#include <QNetworkAccessManager>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

#include <enginioclient.h>
#include <enginioreply.h>

#include "applicationconfig.h"
#include "imageobject.h"
#include "mainwindow.h"
#include "imagemodel.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("Enginio Image Gallery"));

    m_client = new EnginioClient(this);
    m_client->setBackendId(Enginio::BACKEND_ID);
    m_client->setBackendSecret(Enginio::BACKEND_SECRET);

    m_model = new ImageModel(this);
    m_model->setEnginio(m_client);

    m_view = new QListView;
    m_view->setModel(m_model);
    m_view->setViewMode(QListView::IconMode);
    m_view->setGridSize(QSize(104, 104));

    m_fileDialog = new QFileDialog(this);
    m_fileDialog->setFileMode(QFileDialog::ExistingFile);
    m_fileDialog->setNameFilter("Image files (*.png *.jpg *.jpeg)");
    connect(m_fileDialog, SIGNAL(fileSelected(QString)),
            this, SLOT(fileSelected(QString)));

    m_uploadButton = new QPushButton("Upload image");
    connect(m_uploadButton, SIGNAL(clicked()),
            m_fileDialog, SLOT(show()));

    QFrame *frame = new QFrame;
    QVBoxLayout *windowLayout = new QVBoxLayout(frame);
    windowLayout->addWidget(m_view);
    windowLayout->addWidget(m_uploadButton);
    setCentralWidget(frame);

    queryImages();
}

QSize MainWindow::sizeHint() const
{
    return QSize(500, 700);
}

void MainWindow::queryImages()
{
    QJsonObject query;
    query.insert("objectType", QStringLiteral("objects.image"));
    QJsonObject fileObject;
    fileObject.insert("file", QJsonObject());

    // Use include parameter to request full "file" object for every result
    // object including image and thumbnail URLs.
    query.insert("include", fileObject);

    // Filter out objects which have not yet been uploaded completely
    QJsonObject filter;
    QJsonObject exists;
    exists.insert("$exists", true);
    filter.insert("file.id", exists);
    query.insert("query", filter);

    m_model->setQuery(query);
}

void MainWindow::error(EnginioReply *error)
{
    qWarning() << Q_FUNC_INFO << error;
}

void MainWindow::fileSelected(const QString &filePath)
{
    if (filePath.isEmpty())
        return;

    QJsonObject object;
    object.insert("objectType", QString("objects.image"));
    QString fileName = filePath.split(QDir::separator()).last();
    object.insert("name", fileName);
    object.insert("localPath", filePath);
    EnginioReply *reply = m_model->append(object);
    connect(reply, SIGNAL(finished(EnginioReply*)), this, SLOT(beginUpload(EnginioReply*)));
    m_uploads.insert(reply, filePath);
}

void MainWindow::beginUpload(EnginioReply *reply)
{
    reply->deleteLater();
    QString path = m_uploads.take(reply);
    QString objectId = reply->data().value("id").toString();

    QJsonObject object;
    object["id"] = objectId;
    object.insert("objectType", QString("objects.image"));
    object["propertyName"] = QStringLiteral("file");

    QJsonObject fileObject;
    fileObject[QStringLiteral("fileName")] = path;

    QJsonObject uploadJson;
    uploadJson[QStringLiteral("targetFileProperty")] = object;
    uploadJson[QStringLiteral("file")] = fileObject;

    EnginioReply *upload = m_client->uploadFile(uploadJson, QUrl::fromUserInput(path));
    connect(upload, SIGNAL(finished(EnginioReply*)), this, SLOT(uploadFinished(EnginioReply*)));
}

void MainWindow::uploadFinished(EnginioReply *reply)
{
    reply->deleteLater();
}
