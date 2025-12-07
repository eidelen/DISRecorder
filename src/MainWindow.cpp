#include "MainWindow.h"

#include <dis6/EntityStatePdu.h>
#include <QByteArray>
#include <QDataStream>
#include <QDateTime>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHostAddress>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QString>
#include <QCheckBox>
#include <QTimer>
#include <QUdpSocket>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget* parent)
    : QWidget(parent),
      ipEdit_(new QLineEdit("224.0.0.1", this)),
      portEdit_(new QLineEdit("62040", this)),
      destIpEdit_(new QLineEdit("224.0.0.1", this)),
      destPortEdit_(new QLineEdit("62040", this)),
      captureFileEdit_(new QLineEdit(this)),
      replayFileEdit_(new QLineEdit(this)),
      statsLabel_(new QLabel("Packets: 0 | Bytes: 0", this)),
      captureFileButton_(new QPushButton("Browse...", this)),
      replayFileButton_(new QPushButton("Browse...", this)),
      startStopButton_(new QPushButton("Start", this)),
      replayButton_(new QPushButton("Start Replay", this)),
      loopReplayCheck_(new QCheckBox("Loop replay", this)),
      layout_(new QVBoxLayout),
      socket_(new QUdpSocket(this)),
      outputFile_(new QFile(this)),
      replaySocket_(new QUdpSocket(this)),
      replayFile_(new QFile(this)),
      listening_(false),
      replaying_(false),
      packetCount_(0),
      byteCount_(0),
      replayPrevTimestamp_(0),
      replayPort_(0)
{
    setWindowTitle("DIS Recorder");

    connect(captureFileButton_, &QPushButton::clicked, this, &MainWindow::chooseFile);
    connect(replayFileButton_, &QPushButton::clicked, this, &MainWindow::chooseReplayFile);
    connect(startStopButton_, &QPushButton::clicked, this, &MainWindow::startOrStop);
    connect(replayButton_, &QPushButton::clicked, this, &MainWindow::startOrStopReplay);
    connect(socket_, &QUdpSocket::readyRead, this, &MainWindow::handleReadyRead);

    // Touch the OpenDIS library so we verify linkage during the build.
    DIS::EntityStatePdu samplePdu;
    samplePdu.setExerciseID(1);
    const auto marshalledSize = samplePdu.getMarshalledSize();

    auto* info = new QLabel(QStringLiteral("Sample DIS6 marshalled size: %1 bytes").arg(marshalledSize), this);
    info->setAlignment(Qt::AlignCenter);

    statsLabel_->setAlignment(Qt::AlignCenter);

    auto* fileLayout = new QHBoxLayout;
    fileLayout->addWidget(new QLabel("Capture file:", this));
    fileLayout->addWidget(captureFileEdit_);
    fileLayout->addWidget(captureFileButton_);

    auto* replayFileLayout = new QHBoxLayout;
    replayFileLayout->addWidget(new QLabel("Replay file:", this));
    replayFileLayout->addWidget(replayFileEdit_);
    replayFileLayout->addWidget(replayFileButton_);

    auto* netLayout = new QHBoxLayout;
    netLayout->addWidget(new QLabel("Multicast IP:", this));
    netLayout->addWidget(ipEdit_);
    netLayout->addWidget(new QLabel("Port:", this));
    netLayout->addWidget(portEdit_);

    auto* replayNetLayout = new QHBoxLayout;
    replayNetLayout->addWidget(new QLabel("Replay IP:", this));
    replayNetLayout->addWidget(destIpEdit_);
    replayNetLayout->addWidget(new QLabel("Port:", this));
    replayNetLayout->addWidget(destPortEdit_);

    layout_->addLayout(fileLayout);
    layout_->addLayout(netLayout);
    layout_->addLayout(replayFileLayout);
    layout_->addLayout(replayNetLayout);
    layout_->addWidget(statsLabel_);
    layout_->addWidget(info);
    layout_->addWidget(startStopButton_);
    layout_->addWidget(loopReplayCheck_);
    layout_->addWidget(replayButton_);
    setLayout(layout_);

    resize(420, 260);
}

void MainWindow::chooseFile()
{
    const QString path = QFileDialog::getSaveFileName(this, tr("Select capture file"), captureFileEdit_->text());
    if (!path.isEmpty())
    {
        captureFileEdit_->setText(path);
    }
}

void MainWindow::chooseReplayFile()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("Select replay file"), replayFileEdit_->text());
    if (!path.isEmpty())
    {
        replayFileEdit_->setText(path);
    }
}

void MainWindow::startOrStop()
{
    if (listening_)
    {
        stopReceiving();
    }
    else
    {
        if (startReceiving())
        {
            // Status updated inside startReceiving on success.
        }
    }
}

bool MainWindow::startReceiving()
{
    bool ok = false;
    const quint16 port = portEdit_->text().toUShort(&ok);
    if (!ok || port == 0)
    {
        updateStatus("Invalid port.");
        return false;
    }

    QHostAddress address;
    if (!address.setAddress(ipEdit_->text()))
    {
        updateStatus("Invalid IP address.");
        return false;
    }

    if (captureFileEdit_->text().isEmpty())
    {
        updateStatus("Choose a file to record to.");
        return false;
    }

    outputFile_->setFileName(captureFileEdit_->text());
    if (!outputFile_->open(QIODevice::WriteOnly | QIODevice::Append))
    {
        updateStatus("Failed to open file.");
        return false;
    }

    // Reset stats for this session.
    packetCount_ = 0;
    byteCount_ = 0;
    statsLabel_->setText("Packets: 0 | Bytes: 0");

    socket_->close();
    if (!socket_->bind(QHostAddress::AnyIPv4, port, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint))
    {
        updateStatus("Bind failed.");
        outputFile_->close();
        return false;
    }

    if (address.isMulticast())
    {
        if (!socket_->joinMulticastGroup(address))
        {
            updateStatus("Join multicast failed.");
            outputFile_->close();
            socket_->close();
            return false;
        }
    }

    listening_ = true;
    startStopButton_->setText("Stop");
    updateStatus(QStringLiteral("Listening on %1:%2").arg(address.toString()).arg(port));
    return true;
}

void MainWindow::stopReceiving()
{
    if (listening_)
    {
        QHostAddress address;
        if (address.setAddress(ipEdit_->text()) && address.isMulticast())
        {
            socket_->leaveMulticastGroup(address);
        }
        socket_->close();
        outputFile_->close();
        listening_ = false;
        startStopButton_->setText("Start");
        updateStatus("Stopped.");
    }
}

void MainWindow::handleReadyRead()
{
    while (socket_->hasPendingDatagrams())
    {
        QByteArray datagram;
        datagram.resize(int(socket_->pendingDatagramSize()));
        const auto bytesRead = socket_->readDatagram(datagram.data(), datagram.size());
        if (bytesRead > 0 && outputFile_->isOpen())
        {
            QDataStream out(outputFile_);
            out.setByteOrder(QDataStream::BigEndian);
            const qint64 ts = QDateTime::currentMSecsSinceEpoch();
            out << ts << quint32(bytesRead);
            out.writeRawData(datagram.constData(), int(bytesRead));
            packetCount_ += 1;
            byteCount_ += static_cast<quint64>(bytesRead);
            statsLabel_->setText(QStringLiteral("Packets: %1 | Bytes: %2").arg(packetCount_).arg(byteCount_));
        }
    }
}

void MainWindow::updateStatus(const QString& message)
{
    statsLabel_->setText(message);
}

void MainWindow::startOrStopReplay()
{
    if (replaying_)
    {
        stopReplay();
    }
    else
    {
        startReplay();
    }
}

bool MainWindow::startReplay()
{
    if (listening_)
    {
        stopReceiving();
    }

    replayAddress_ = QHostAddress();
    if (!replayAddress_.setAddress(destIpEdit_->text()))
    {
        updateStatus("Invalid replay IP address.");
        return false;
    }

    bool ok = false;
    replayPort_ = destPortEdit_->text().toUShort(&ok);
    if (!ok || replayPort_ == 0)
    {
        updateStatus("Invalid replay port.");
        return false;
    }

    if (replayFileEdit_->text().isEmpty())
    {
        updateStatus("Choose a capture file to replay.");
        return false;
    }

    replayFile_->setFileName(replayFileEdit_->text());
    if (!replayFile_->open(QIODevice::ReadOnly))
    {
        updateStatus("Failed to open replay file.");
        return false;
    }

    replayStream_.setDevice(replayFile_);
    replayStream_.setByteOrder(QDataStream::BigEndian);
    replayPrevTimestamp_ = 0;
    replayBuffer_.clear();

    qint64 ts = 0;
    QByteArray payload;
    if (!readNextReplayPacket(ts, payload))
    {
        updateStatus("Capture file is empty.");
        stopReplay();
        return false;
    }

    replayPrevTimestamp_ = ts;
    replayBuffer_ = payload;
    replaying_ = true;
    replayButton_->setText("Stop Replay");
    updateStatus("Replaying...");

    QTimer::singleShot(0, this, &MainWindow::handleReplaySend);
    return true;
}

void MainWindow::stopReplay()
{
    if (replaying_)
    {
        replayFile_->close();
        replaying_ = false;
        replayButton_->setText("Start Replay");
        updateStatus("Replay stopped.");
    }
}

void MainWindow::handleReplaySend()
{
    if (!replaying_)
    {
        return;
    }

    // Send current packet.
    replaySocket_->writeDatagram(replayBuffer_, replayAddress_, replayPort_);

    qint64 nextTs = 0;
    QByteArray nextPayload;
    if (!readNextReplayPacket(nextTs, nextPayload))
    {
        if (loopReplayCheck_->isChecked())
        {
            // Restart from beginning.
            replayFile_->seek(0);
            replayPrevTimestamp_ = 0;
            if (!readNextReplayPacket(nextTs, nextPayload))
            {
                updateStatus("Replay completed (file empty).");
                stopReplay();
                return;
            }
        }
        else
        {
            updateStatus("Replay completed.");
            stopReplay();
            return;
        }
    }

    const qint64 delayMs = qMax<qint64>(0, nextTs - replayPrevTimestamp_);
    replayPrevTimestamp_ = nextTs;
    replayBuffer_ = nextPayload;
    QTimer::singleShot(int(delayMs), this, &MainWindow::handleReplaySend);
}

bool MainWindow::readNextReplayPacket(qint64& timestampMs, QByteArray& payload)
{
    if (!replayStream_.device())
    {
        return false;
    }

    if (replayStream_.atEnd())
    {
        return false;
    }

    quint32 size = 0;
    replayStream_ >> timestampMs >> size;

    if (replayStream_.status() != QDataStream::Ok || size == 0 || size > 65535)
    {
        return false;
    }

    payload.resize(int(size));
    const int read = replayStream_.readRawData(payload.data(), int(size));
    if (read != int(size))
    {
        return false;
    }

    return true;
}
