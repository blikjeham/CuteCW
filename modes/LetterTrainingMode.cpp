#include "LetterTrainingMode.h"
#include "Morse.h"

#include <qdebug.h>

LetterTrainingMode::LetterTrainingMode(Morse *parent, Ui::MainWindow *ui)
    : TrainingMode(parent, ui)
{
}

void LetterTrainingMode::switchToMode() {
    m_ui->letter->show();
    m_ui->play->show();
    setupTrainingWidgets();
    m_ui->changeSequence->show();
    m_ui->helpBar->setText("<font color=\"green\">Type the letter you hear as accurately as possible within a half a second.</font>");

    setupSequences();
    setupWidgets(m_trainingSequence);
    clear();
}

void LetterTrainingMode::handleKeyPress(QChar letterPressed) {
    if (runningMode() != RUNNING)
        return;

    // analyze they're keyed letter and immediately start playing a new one
    TrainingMode::handleKeyPress(letterPressed);
    startNextTrainingKey();
}

QString LetterTrainingMode::helpText()
{
    return tr("<p>In this mode characters will be played for you. You will need to press the appropriate within a half-a-second.  Accuracy is more important than speed."
              "In order to continue advancing in the character set you will need to achieve at least a 90% accuracy rate on the letters, with at least 5 guesses for each"
              "letter under your belt."
              "Characters that you need the most help with will be sent more frequently than the characters you're good at.</p>"
              "<p>The important lesson is to learn to recognize and automatically type the letter you hear, without stressing out about the next letter. Be <b>sure</b> "
              "to aim for accuracy not speed in this mode.</p>"
              "<p>After you succeed in learning the characters, continue on to the speed training mode next where you'll be judged on both accuracy and speed.");
}

QString LetterTrainingMode::name()
{
    return tr("Recognition Training");
}

QTime LetterTrainingMode::startNextTrainingKey() {
    if (runningMode() != RUNNING)
        return QTime();
    qDebug() << "--- Start next training key";
    int letterCount = 0;
    QList<QPair<QChar, int> > letters;
    int totalPercent = 0, thisPercent = 0, minPercent = 0;
    MorseStat *stat = 0;
    QString currentLetterGoal;
    int badLetters = 0;

    //if (m_morse->audioMode() == Morse::PLAYING)
    //    return;

    QString::iterator letter;
    QString::iterator lastLetter = m_trainingSequence.end();
    for(letter = m_trainingSequence.begin(); letter != lastLetter; ++letter) {
        letterCount++;
        currentLetterGoal = (*letter).toUpper();
        stat = getStat(*letter);
        thisPercent = stat->getGoodPercentage();
        totalPercent += thisPercent;
        if (minPercent > thisPercent)
            minPercent = thisPercent;
        if (thisPercent == 0) {
            if (m_doEntireSequence) {
                thisPercent = 50;
            } else {
                // never keyed yet; do it immediately if we got this far
                qDebug() << "|keying: " << *letter;
                m_lastKey = *letter;
                m_lastKeys.append(*letter);
                setSequence(m_trainingSequence, letterCount);
                m_ui->avewpm->setText("All Percentage: " + QString().setNum(totalPercent/letterCount) + ", " +
                                      *letter + ": NEW");
                setWPMLabel(thisPercent);
                m_lastTimes.push_back(m_morse->playIt(*letter));
                updateGraphs();
                return m_lastTimes.last();
            }
        }

        qDebug() << "  adding " << *letter << " / " << thisPercent;
        letters.append(QPair<QChar, int>(*letter, thisPercent));

        if(thisPercent <= m_percentGoal || stat->getTryCount() <= m_minimumTries) {
            qDebug() << "   too low: " << *letter << " / " << thisPercent << " / " << stat->getTryCount();
            if (++badLetters >= m_maxBadLetters || stat->getTryCount() <= m_minimumTries) {
                // enough letters aren't accurate; break here
                break;
            }
        }
    }

    // They have the whole sequence active at this point

    m_ui->avewpm->setText("All Percentage: " + QString().setNum(totalPercent/letterCount) + ", " +
                          *letter + ": NEW");
    setWPMLabel(totalPercent/letterCount);
    // now pick a random time between 0 and the total of all the averages; averages with a slower speed are more likely
    // XXX: probably could use a weighted average (subtract off min speed from all speeds)?

    float randPercent;
    int subPercent = 0;
    if (m_morse->badLetterWeighting() == Morse::HIGH) {
        subPercent = minPercent/2;
        randPercent = (totalPercent - subPercent * letters.count())*float(qrand())/float(RAND_MAX);
    } else
        randPercent = totalPercent*float(qrand())/float(RAND_MAX);
    float newTotal = 0;
    // qDebug() << "letter set random: " << randPercent << " total: " << totalPercent << " min: " << minPercent/2 << ", count: " << letters.count();
    QList<QPair<QChar, int> >::iterator search;
    QList<QPair<QChar, int> >::iterator last = letters.end();
    setSequence(m_trainingSequence, letterCount);
    for(search = letters.begin(); search != last; ++search) {
        //qDebug() << "  -> " << (*search).first << "/" << (*search).second;
        newTotal += ((*search).second - subPercent);
        if (newTotal > randPercent) {
            qDebug() << ">keying: " << (*search).first;
            m_lastKey = (*search).first;
            m_lastKeys.append((*search).first);
            m_lastTimes.push_back(m_morse->playIt((*search).first));
            updateGraphs();
            return m_lastTimes.last();
        }
    }
    qDebug() << "**** shouldn't get here: " << randPercent << "," << totalPercent;
    return QTime();
}

bool LetterTrainingMode::elapsedTimeWasTooLong(int msElapsed, MorseStat *stat) {
    Q_UNUSED(stat);
    if (msElapsed > 500) {
        return true;
    }
    return false;
}

void LetterTrainingMode::updateGraphs()
{
#ifndef SMALL_DEVICE
    foreach(QChar theLetter, m_trainingSequence) {
        m_progressBars[theLetter]->setRange(0, 100);
        m_progressBars[theLetter]->setValue(getStat(theLetter)->getGoodPercentage());
    }
    // qDebug() << "max graph WPM: " << fastestWPM;
#endif
}
