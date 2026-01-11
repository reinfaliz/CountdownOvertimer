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
#include <QDir>

class TimerApp : public QWidget {
    Q_OBJECT

public:
    TimerApp(QWidget *parent = nullptr) : QWidget(parent) {
        setWindowTitle("Countdown Overtimer");
        resize(400, 300);

        loadConfig();
        setupUI();
        resetTimer(); // Initialize logic
    }

private:
    // --- Configuration Variables ---
    int startMin = 0;
    int startSec = 10;
    int limitMin = 0;
    int limitSec = 10;
    QString soundZeroFile;
    QString soundLimitFile;

    // --- State Variables ---
    int currentTotalSeconds;
    int limitTotalSeconds;
    bool isRunning = false;
    bool isPaused = false;

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
            QMessageBox::warning(this, "Config Error", "Could not open config.txt. Using defaults.");
            return;
        }

        QTextStream in(&file);
        
        // --- Helper Lambda to read and clean lines ---
        auto readValidLine = [&]() -> QString {
            while (!in.atEnd()) {
                QString line = in.readLine();
                
                // 1. Remove comments (split at '#' and take the first part)
                QStringList parts = line.split('#');
                QString cleanLine = parts[0];

                // 2. Trim whitespace
                cleanLine = cleanLine.trimmed();

                // 3. If line is not empty, return it. If empty, loop again.
                if (!cleanLine.isEmpty()) {
                    return cleanLine;
                }
            }
            return QString(); // Return empty if end of file
        };

        // --- Read values in specific order ---
        // We use toInt() which returns 0 on failure, which is a safe fallback
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

        // 3. Setup Timer
        timer = new QTimer(this);
        timer->setInterval(1000); // 1 second
        connect(timer, &QTimer::timeout, this, &TimerApp::onTick);

        // 4. Setup Audio
        player = new QMediaPlayer(this);
        audioOutput = new QAudioOutput(this);
        player->setAudioOutput(audioOutput);
        audioOutput->setVolume(1.0); 
    }

    void updateDisplay() {
        int absSeconds = std::abs(currentTotalSeconds);
        int m = absSeconds / 60;
        int s = absSeconds % 60;

        QString sign = (currentTotalSeconds < 0) ? "-" : "";
        QString timeStr = QString("%1%2:%3")
                            .arg(sign)
                            .arg(m, 2, 10, QChar('0'))
                            .arg(s, 2, 10, QChar('0'));

        lblDisplay->setText(timeStr);

        if (currentTotalSeconds < 0) {
            lblDisplay->setStyleSheet("color: red;");
        } else {
            lblDisplay->setStyleSheet("color: black;");
        }
    }

    void playSound(const QString &fileName) {
        if (fileName.isEmpty()) return;

        QFileInfo checkFile(fileName);
        if (checkFile.exists() && checkFile.isFile()) {
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

        currentTotalSeconds = (startMin * 60) + startSec;
        limitTotalSeconds = -1 * ((limitMin * 60) + limitSec);

        btnStartPause->setText("Start");
        btnStartPause->setVisible(true);
        updateDisplay();
    }

    void onStartPauseClicked() {
        if (!isRunning) {
            isRunning = true;
            isPaused = false;
            btnStartPause->setText("Pause");
            timer->start();
        } else {
            isRunning = false;
            isPaused = true;
            btnStartPause->setText("Start");
            timer->stop();
        }
    }

    void onTick() {
        currentTotalSeconds--;
        updateDisplay();

        if (currentTotalSeconds == 0) {
            playSound(soundZeroFile);
        }

        if (currentTotalSeconds == limitTotalSeconds) {
            playSound(soundLimitFile);
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
