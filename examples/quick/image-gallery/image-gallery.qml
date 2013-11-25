/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
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
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
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

import QtQuick 2.0
import Enginio 1.0

import QtQuick.Dialogs 1.0
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0

/*
 * Enginio image gallery example.
 *
 * Main window contains list of enginioModel on the backend and button for uploading
 * new enginioModel. Image list contains image thumbnail (generated by Enginio
 * backend) and some image metadata. Clicking list items downloads image file
 * and displays it in dialog window. Clicking the red x deletes enginioModel from
 * backend.
 *
 * In the backend enginioModel are represented as objects of type "objects.image". These
 * objects contain a property "file" which is a reference to the actual binary file.
 */

Rectangle {
    id: root

    width: 460
    height: 640
    color: "#f4f4f4"

    property var imagesUrl: new Object

    // Enginio client specifies the backend to be used
    //! [client]
    EnginioClient {
        id: client
        backendId: enginioBackendId
        onError: console.log("Enginio error: " + reply.errorCode + ": " + reply.errorString)
    }
    //! [client]

    //! [model]
    EnginioModel {
        id: enginioModel
        client: client
        query: { // query for all objects of type "objects.image" and include not null references to files
            "objectType": "objects.image",
                    "include": {"file": {}},
            "query" : { "file": { "$ne": null } }
        }
    }
    //! [model]

    // Delegate for displaying individual rows of the model
    Component {
        id: imageListDelegate

        BorderImage {
            height: 120
            width: parent.width
            border.top: 4
            border.bottom: 4
            source: hitbox.pressed ? "qrc:images/delegate_pressed.png" : "qrc:images/delegate.png"

            Image {
                id: shadow
                anchors.top: parent.bottom
                width: parent.width
                visible: !hitbox.pressed
                source: "qrc:images/shadow.png"
            }

            //! [image-fetch]
            Image {
                id: image
                x: 10
                width: 100
                height: 100
                anchors.verticalCenter: parent.verticalCenter
                opacity: image.status == Image.Ready ? 1 : 0
                Behavior on opacity { NumberAnimation { duration: 100 } }
                Component.onCompleted: {
                    if (id in imagesUrl) {
                        image.source = imagesUrl[id]
                    } else {
                        var data = { "id": file.id,
                            "variant": "thumbnail"}
                        var reply = client.downloadUrl(data)
                        reply.finished.connect(function() {
                            imagesUrl[id] = reply.data.expiringUrl
                            if (image && reply.data.expiringUrl) // It may be deleted as it is delegate
                                image.source = reply.data.expiringUrl
                        })
                    }
                }
            }
            Rectangle {
                color: "transparent"
                anchors.fill: image
                border.color: "#aaa"
                Rectangle {
                    id: progressBar
                    property real value:  image.progress
                    anchors.bottom: parent.bottom
                    width: image.width * value
                    height: 4
                    color: "#49f"
                    opacity: image.status != Image.Ready ? 1 : 0
                    Behavior on opacity {NumberAnimation {duration: 100}}
                }
            }
            //! [image-fetch]

            Column {
                anchors.left: image.right
                anchors.right: deleteIcon.left
                anchors.margins: 12
                y: 10
                Text {
                    height: 33
                    width: parent.width
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: height * 0.5
                    text: name ? name : ""
                    elide: Text.ElideRight
                }
                Text {
                    height: 33
                    width: parent.width
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: height * 0.5
                    text: sizeStringFromFile(file)
                    elide:Text.ElideRight
                    color: "#555"
                }
                Text {
                    height: 33
                    width: parent.width
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: height * 0.5
                    text: timeStringFromFile(file)
                    elide:Text.ElideRight
                    color: "#555"
                }
            }

            // Clicking list item opens full size image in separate dialog
            MouseArea {
                id: hitbox
                anchors.fill: parent
                onClicked: {
                    imageDialog.fileId = file.id;
                    imageDialog.visible = true
                    root.state = "view"
                }
            }

            // Delete button
            Image {
                id: deleteIcon
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.rightMargin: 18
                source: removeMouseArea.pressed ?"qrc:icons/delete_icon_pressed.png" : "qrc:icons/delete_icon.png"
                MouseArea {
                    id: removeMouseArea
                    anchors.fill: parent
                    anchors.margins: -10
                    onClicked: enginioModel.remove(index)
                }
            }
        }
    }

    // A simple layout:
    // a listview and a line edit with button to add to the list
    Rectangle {
        id: header
        anchors.top: parent.top
        width: parent.width
        height: 70
        color: "white"

        Row {
            id: logo
            anchors.centerIn: parent
            anchors.horizontalCenterOffset: -4
            spacing: 6
            Image {
                source: "qrc:images/enginio.png"
                width:160 ; height: 60
                fillMode:Image.PreserveAspectFit
            }
            Text {
                text: "Gallery"
                anchors.verticalCenter: parent.verticalCenter
                anchors.verticalCenterOffset: -3
                font.bold: true
                font.pixelSize: 46
                color: "#555"
            }
        }
        Rectangle {
            width: parent.width ; height: 1
            anchors.bottom: parent.bottom
            color: "#bbb"
        }
    }

    Row {
        id: listLayout

        Behavior on x {NumberAnimation{ duration: 400 ; easing.type: "InOutCubic"}}
        anchors.top: header.bottom
        anchors.bottom: footer.top

        ListView {
            id: imageListView
            model: enginioModel // get the data from EnginioModel
            delegate: imageListDelegate
            clip: true
            width: root.width
            height: parent.height
            // Animations
            add: Transition { NumberAnimation { properties: "y"; from: root.height; duration: 250 } }
            removeDisplaced: Transition { NumberAnimation { properties: "y"; duration: 150 } }
            remove: Transition { NumberAnimation { property: "opacity"; to: 0; duration: 150 } }
        }

        // Dialog for showing full size image
        Rectangle {
            id: imageDialog
            width: root.width
            height: parent.height
            property string fileId
            color: "#333"

            onFileIdChanged: {
                image.source = ""
                // Download the full image, not the thumbnail
                var data = { "id": fileId }
                var reply = client.downloadUrl(data)
                reply.finished.connect(function() {
                    image.source = reply.data.expiringUrl
                })
            }
            Label {
                id: label
                text: "Loading ..."
                font.pixelSize: 28
                color: "white"
                anchors.centerIn: parent
                visible: image.status != Image.Ready
            }
            Rectangle {
                property real value: image.progress
                anchors.bottom: parent.bottom
                width: parent.width * value
                height: 4
                color: "#49f"
                Behavior on opacity {NumberAnimation {duration: 200}}
                opacity: image.status !== Image.Ready ? 1 : 0
            }
            Image {
                id: image
                anchors.fill: parent
                anchors.margins: 10
                smooth: true
                cache: false
                fillMode: Image.PreserveAspectFit
                Behavior on opacity { NumberAnimation { duration: 100 } }
                opacity: image.status === Image.Ready ? 1 : 0
            }
            MouseArea {
                anchors.fill: parent
                onClicked: root.state = ""
            }
        }
    }

    BorderImage {
        id: footer

        width: parent.width
        anchors.bottom: parent.bottom
        source: addMouseArea.pressed ? "qrc:images/delegate_pressed.png" : "qrc:images/delegate.png"
        border.left: 5; border.top: 5
        border.right: 5; border.bottom: 5

        Rectangle {
            y: -1 ; height: 1
            width: parent.width
            color: "#bbb"
        }
        Rectangle {
            y: 0 ; height: 1
            width: parent.width
            color: addMouseArea.pressed ? "transparent" : "white"
        }

        //![append]

        Text {
            text: "Click to upload..."
            font.bold: true
            font.pixelSize: 28
            color: "#444"
            anchors.centerIn: parent
        }

        Item {
            id: addButton
            width: 40 ; height: 40
            anchors.margins: 20
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            Image {
                id: removeIcon
                source: addMouseArea.pressed ? "qrc:icons/add_icon_pressed.png" : "qrc:icons/add_icon.png"
                anchors.centerIn: parent
            }
        }

        MouseArea {
            id: addMouseArea
            anchors.fill: parent
            onClicked: fileDialog.visible = true;
        }
        Rectangle {
            id: progressBar
            property real value:0
            anchors.bottom: parent.bottom
            width: parent.width * value
            height: 4
            color: "#49f"
            Behavior on opacity {NumberAnimation {duration: 100}}
        }
    }

    // File dialog for selecting image file from local file system
    FileDialog {
        id: fileDialog
        title: "Select image file to upload"
        nameFilters: [ "Image files (*.png *.jpg *.jpeg)", "All files (*)" ]

        onSelectionAccepted: {
            var pathParts = fileUrl.toString().split("/");
            var fileName = pathParts[pathParts.length - 1];
            var fileObject = {
                objectType: "objects.image",
                name: fileName,
                localPath: fileUrl.toString()
            }
            var reply = client.create(fileObject);
            reply.finished.connect(function() {
                var uploadData = {
                    file: { fileName: fileName },
                    targetFileProperty: {
                        objectType: "objects.image",
                        id: reply.data.id,
                        propertyName: "file"
                    },
                };

                imagesUrl[reply.data.id] = reply.data.localPath

                var uploadReply = client.uploadFile(uploadData, fileUrl)
                progressBar.opacity = 1
                uploadReply.progress.connect(function(progress, total) {
                    progressBar.value = progress/total
                })
                uploadReply.finished.connect(function() {
                    var tmp = enginioModel.query; enginioModel.query = {}; enginioModel.query = tmp;
                    progressBar.opacity = 0
                })
            })
        }
    }

    function sizeStringFromFile(fileData) {
        var str = [];
        if (fileData && fileData.fileSize) {
            str.push("Size: ");
            str.push(fileData.fileSize);
            str.push(" bytes");
        }
        return str.join("");
    }

    function doubleDigitNumber(number) {
        if (number < 10)
            return "0" + number;
        return number;
    }

    function timeStringFromFile(fileData) {
        var str = [];
        if (fileData && fileData.createdAt) {
            var date = new Date(fileData.createdAt);
            if (date) {
                str.push("Uploaded: ");
                str.push(date.toDateString());
                str.push(" ");
                str.push(doubleDigitNumber(date.getHours()));
                str.push(":");
                str.push(doubleDigitNumber(date.getMinutes()));
            }
        }
        return str.join("");
    }

    states: [
        State {
            name: "view"
            PropertyChanges {
                target: listLayout
                x: -root.width
            }
        }
    ]

}
