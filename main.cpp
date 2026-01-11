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
#include <QKeyEvent>    // Added for Keyboard Input
#include <QResizeEvent> // Added for Window Resizing
#include <cmath>

class TimerApp : public QWidget {
    Q_OBJECT

public:
    TimerApp(QWidget *parent = nullptr) : QWidget(parent) {
        setWindowTitle("Negative Countdown Timer");
        resize(600, 400); // Slightly larger default start size

        loadConfig();
        setupUI();
        resetTimer(); 
    }

protected:
    // (1) Handle Window Resizing (Scaling the Text)
    void resizeEvent(QResizeEvent *event) override {
        QWidget::resizeEvent(event);
        
        // Calculate new font size dynamically
        // We want the text to fit within the window, regardless of aspect ratio.
        // "00:00" is 5 chars, "-15:00" is 6 chars. 
        
        int h = height();
        int w = width();

        // Heuristic: 
        // 1. Height-based: Text should take up about 50% of screen height
        // 2. Width-based: Width / 5 (roughly chars count)
        int sizeByHeight = h * 0.50; 
        int sizeByWidth = w / 4.5;   

        // Pick the smaller one to ensure it fits inside the window
        int newPointSize = std::min(sizeByHeight, sizeByWidth);

        // Set a minimum safety size so it doesn't disappear
        if (newPointSize < 20) newPointSize = 20;

        QFont font = lblDisplay->font();
        font.setPointSize(newPointSize);
        lblDisplay->setFont(font);
    }

    // (2) Handle Alt + Enter for Fullscreen
    void keyPressEvent(QKeyEvent *event) override {
        // Check for Alt + Enter (Key_Return is the main Enter, Key_Enter is Numpad Enter)
        if ((event->modifiers() & Qt::AltModifier) && 
            (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)) {
            
            if (isFullScreen()) {
                showNormal();
            } else {
                showFullScreen();
            }
            // Note: The resizeEvent() is called automatically by Qt 
            // when the window state changes, so the text will scale automatically.
            
        } else {
            // Pass other keys to the parent class
            QWidget::keyPressEvent(event);
        }
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
    qint64 currentMs;       
    qint64 limitMs;         
    qint64 targetEndTime;   
    bool isRunning = false;
    bool isPaused = false;
    bool zeroSoundPlayed = false; 

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
        font.setBold(true);
        lblDisplay->setFont(font);
        
        // Allow the label to expand to fill available space
        lblDisplay->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        
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

        // 3. Timer Setup
        timer = new QTimer(this);
        timer->setInterval(50); 
        connect(timer, &QTimer::timeout, this, &TimerApp::onTick);

        // 4. Audio Setup
        player = new QMediaPlayer(this);
        audioOutput = new QAudioOutput(this);
        player->setAudioOutput(audioOutput);
        audioOutput->setVolume(1.0); 
    }

    void updateDisplay() {
        qint64 absMs = std::abs(currentMs);
        qint64 totalSeconds = absMs / 1000; 

        int m = totalSeconds / 60;
        int s = totalSeconds % 60;

        QString sign = (currentMs < 0) ? "-" : "";
        
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
            player->stop(); 
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
        player->stop();
        isRunning = false;
        isPaused = false;
        zeroSoundPlayed = false;

        currentMs = ((startMin * 60) + startSec) * 1000;
        limitMs = -1 * ((limitMin * 60) + limitSec) * 1000;

        btnStartPause->setText("Start");
        btnStartPause->setVisible(true);
        updateDisplay();
        
        // Force a resize event to ensure font is correct size on startup/reset
        QResizeEvent *event = new QResizeEvent(this->size(), this->size());
        QCoreApplication::postEvent(this, event);
    }

    void onStartPauseClicked() {
        if (!isRunning) {
            isRunning = true;
            isPaused = false;
            btnStartPause->setText("Pause");
            targetEndTime = QDateTime::currentMSecsSinceEpoch() + currentMs;
            timer->start();
        } else {
            isRunning = false;
            isPaused = true;
            btnStartPause->setText("Start");
            timer->stop();
        }
    }

    void onTick() {
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        currentMs = targetEndTime - now;
        
        updateDisplay();

        if (currentMs <= 0 && !zeroSoundPlayed) {
            playSound(soundZeroFile);
            zeroSoundPlayed = true;
        }

        if (currentMs <= limitMs) {
            currentMs = limitMs; 
            updateDisplay();
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

