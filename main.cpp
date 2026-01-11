#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QFile>
#include <QTextStream>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QUrl>
#include <QMessageBox>
#include <QFileInfo>
#include <QDateTime> 
#include <cmath>

class TimerApp : public QWidget {
    Q_OBJECT

public:
    TimerApp(QWidget *parent = nullptr) : QWidget(parent) {
        setWindowTitle("Countdown Overtimer");
        resize(400, 300);

        loadConfig();
        setupUI();
        resetTimer(); 
    }

private:
    // --- Configuration Variables ---
    int startMin = 0;
    int startSec = 10;
    int limitMin = 0;
    int limitSec = 10;
    QString soundZeroFile;
    QString soundLimitFile;

    // --- State Variables (Now using Milliseconds for precision) ---
    qint64 currentMs;       // Current remaining time in ms
    qint64 limitMs;         // Limit time in ms (negative)
    qint64 targetEndTime;   // The system clock time when the timer should finish
    bool isRunning = false;
    bool isPaused = false;
    bool zeroSoundPlayed = false; // Flag to ensure zero sound plays only once

    // --- GUI Components ---
    QLabel *lblDisplay;
    QPushButton *btnStartPause;
    QPushButton *btnReset;
    QTimer *timer;
    
    // --- Audio Components ---
    QMediaPlayer *player;
    QAudioOutput *audioOutput;

    void loadConfig() {
        QFile file("config.txt");
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            // If config fails, use defaults silently or warn
            return; 
        }

        QTextStream in(&file);
        
        auto readValidLine = [&]() -> QString {
            while (!in.atEnd()) {
                QString line = in.readLine();
                QString cleanLine = line.split('#')[0].trimmed();
                if (!cleanLine.isEmpty()) return cleanLine;
            }
            return QString();
        };

        startMin = readValidLine().toInt();
        startSec = readValidLine().toInt();
        limitMin = readValidLine().toInt();
        limitSec = readValidLine().toInt();
        soundZeroFile = readValidLine();
        soundLimitFile = readValidLine();
        
        file.close();
    }

    void setupUI() {
        QVBoxLayout *mainLayout = new QVBoxLayout(this);

        // 1. Timer Display
        lblDisplay = new QLabel("00:00", this);
        lblDisplay->setAlignment(Qt::AlignCenter);
        QFont font = lblDisplay->font();
        font.setPointSize(60);
        font.setBold(true);
        lblDisplay->setFont(font);
        mainLayout->addWidget(lblDisplay);

        // 2. Button Layout
        QHBoxLayout *btnLayout = new QHBoxLayout();
        
        btnStartPause = new QPushButton("Start", this);
        btnStartPause->setMinimumHeight(40);
        connect(btnStartPause, &QPushButton::clicked, this, &TimerApp::onStartPauseClicked);
        btnLayout->addWidget(btnStartPause);

        btnReset = new QPushButton("Reset", this);
        btnReset->setMinimumHeight(40);
        connect(btnReset, &QPushButton::clicked, this, &TimerApp::onResetClicked);
        btnLayout->addWidget(btnReset);

        mainLayout->addLayout(btnLayout);

        // 3. Setup Update Timer (Runs frequently for responsiveness)
        timer = new QTimer(this);
        timer->setInterval(50); // Update UI every 50ms
        connect(timer, &QTimer::timeout, this, &TimerApp::onTick);

        // 4. Setup Audio
        player = new QMediaPlayer(this);
        audioOutput = new QAudioOutput(this);
        player->setAudioOutput(audioOutput);
        audioOutput->setVolume(1.0); 
    }

    void updateDisplay() {
        // Convert ms to seconds for display
        // We use absolute value because formatting handles the sign
        qint64 absMs = std::abs(currentMs);
        qint64 totalSeconds = absMs / 1000; 

        int m = totalSeconds / 60;
        int s = totalSeconds % 60;

        QString sign = (currentMs < 0) ? "-" : "";
        
        // Handle the special case where it is negative zero (e.g. -0.5s)
        // If currentMs is between -999 and 0, we force a negative sign
        if (currentMs < 0 && currentMs > -1000) {
            sign = "-";
        }

        QString timeStr = QString("%1%2:%3")
                            .arg(sign)
                            .arg(m, 2, 10, QChar('0'))
                            .arg(s, 2, 10, QChar('0'));

        lblDisplay->setText(timeStr);

        if (currentMs < 0) {
            lblDisplay->setStyleSheet("color: red;");
        } else {
            lblDisplay->setStyleSheet("color: black;");
        }
    }

    void playSound(const QString &fileName) {
        if (fileName.isEmpty()) return;

        QFileInfo checkFile(fileName);
        if (checkFile.exists() && checkFile.isFile()) {
            player->stop(); // Stop any currently playing sound
            player->setSource(QUrl::fromLocalFile(checkFile.absoluteFilePath()));
            player->play();
        } else {
            QApplication::beep();
        }
    }

private slots:
    void onResetClicked() {
        resetTimer();
    }

    void resetTimer() {
        timer->stop();
        isRunning = false;
        isPaused = false;
        zeroSoundPlayed = false;

        // Calculate initial ms
        currentMs = ((startMin * 60) + startSec) * 1000;
        
        // Calculate limit ms (negative)
        limitMs = -1 * ((limitMin * 60) + limitSec) * 1000;

        btnStartPause->setText("Start");
        btnStartPause->setVisible(true);
        updateDisplay();
    }

    void onStartPauseClicked() {
        if (!isRunning) {
            // --- STARTING ---
            isRunning = true;
            isPaused = false;
            btnStartPause->setText("Pause");
            
            // Calculate the target end time based on current system clock
            // Target = Now + TimeRemaining
            targetEndTime = QDateTime::currentMSecsSinceEpoch() + currentMs;
            
            timer->start();
        } else {
            // --- PAUSING ---
            isRunning = false;
            isPaused = true;
            btnStartPause->setText("Start");
            timer->stop();
            
            // Note: We don't need to do anything else.
            // 'currentMs' holds the exact milliseconds remaining 
            // because it was updated in the last onTick().
        }
    }

    void onTick() {
        // Calculate remaining time based on system clock
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        currentMs = targetEndTime - now;
        
        updateDisplay();

        // 1. Check for 00:00
        // We check if we just crossed zero or are very close to it
        // Using a flag ensures we only play it once
        if (currentMs <= 0 && !zeroSoundPlayed) {
            playSound(soundZeroFile);
            zeroSoundPlayed = true;
        }

        // 2. Check for Limit
        if (currentMs <= limitMs) {
            // Clamp to limit so display shows exact limit
            currentMs = limitMs; 
            updateDisplay();
            
            playSound(soundLimitFile);
            
            // Stop everything
            timer->stop();
            isRunning = false;
            isPaused = true;
            btnStartPause->setVisible(false);
        }
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    TimerApp window;
    window.show();
    return app.exec();
}

#include "main.moc"
