#pragma once

#include <QByteArray>
#include <QDataStream>
#include <QHostAddress>
#include <QWidget>

class QFile;
class QFileDialog;
class QLabel;
class QLineEdit;
class QCheckBox;
class QPushButton;
class QUdpSocket;
class QString;
class QVBoxLayout;

class MainWindow : public QWidget {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void chooseFile();
    void startOrStop();
    bool startReceiving();
    void stopReceiving();
    void handleReadyRead();
    void updateStatus(const QString &message);
    void chooseReplayFile();
    void startOrStopReplay();
    bool startReplay();
    void stopReplay();
    void handleReplaySend();
    bool readNextReplayPacket(qint64 &timestampMs, QByteArray &payload);

    QLineEdit *ipEdit_;
    QLineEdit *portEdit_;
    QLineEdit *destIpEdit_;
    QLineEdit *destPortEdit_;
    QLineEdit *captureFileEdit_;
    QLineEdit *replayFileEdit_;
    QLabel *statsLabel_;
    QPushButton *captureFileButton_;
    QPushButton *replayFileButton_;
    QPushButton *startStopButton_;
    QPushButton *replayButton_;
    QCheckBox *loopReplayCheck_;
    QVBoxLayout *layout_;

    QUdpSocket *socket_;
    QFile *outputFile_;
    QDataStream replayStream_;
    QUdpSocket *replaySocket_;
    QFile *replayFile_;
    bool listening_;
    bool replaying_;
    quint64 packetCount_;
    quint64 byteCount_;
    qint64 replayPrevTimestamp_;
    QByteArray replayBuffer_;
    QHostAddress replayAddress_;
    quint16 replayPort_;
};
