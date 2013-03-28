#include "faceinvaderswidget.h"
#include <QLinearGradient>
#include <QDebug>
#include <QGraphicsEllipseItem>
#include <QTimer>
#include <QTime>
#include <QThread>
#include <cstdlib>
#include <QFont>
#include <QGLWidget>

const QRect FaceInvadersScene::DefaultGameSize = QRect(0,0, 600, 440);

FaceInvadersScene::FaceInvadersScene(QWidget *parent):
    QGraphicsScene(parent), player(new PlayerItem()), m_scoreItem(new QGraphicsSimpleTextItem()),
    m_gameState(InitState), m_gameScore(0), m_invaderScale(1.0), m_invaderSpeed(1)
{
    this->setBackgroundBrush(QBrush(Qt::black));
    background = new QImage(QString(":/images/background2.png"));

    m_gameSize = FaceInvadersScene::DefaultGameSize;

    m_playerStartingPosition.setX(280);
    m_playerStartingPosition.setY(360);
    this->addItem(player);
    player->setPos(m_playerStartingPosition);
    connect(player, SIGNAL(PlayerHit()), this, SLOT(endGame()));

    QFont f("Helvetica", 18, QFont::Bold);
    m_scoreItem->setFont(f);
    QBrush brush(Qt::yellow);
    QPen pen(Qt::black);
    m_scoreItem->setBrush(brush);
    m_scoreItem->setPen(pen);

    alienEvaded(0);
    updateScorePosition();
    this->addItem(m_scoreItem);
    m_scoreItem->setZValue(1.0f);

    m_advanceTimer = new QTimer(this);
    connect(m_advanceTimer, SIGNAL(timeout()), this, SLOT(advance()));

    //Init rand() seed
    int seed = QTime::currentTime().msec() * QTime::currentTime().second();
    srand(QThread::currentThreadId() * seed);
}

FaceInvadersScene::~FaceInvadersScene()
{
    delete background;
    delete m_advanceTimer;
}

QRectF FaceInvadersScene::getGameScreenSize()
{
    return m_gameSize;
}

void FaceInvadersScene::setGameScreenSize(QRectF &rect)
{
    m_gameSize = rect;
    updateScorePosition();
    //! \todo Make sure everything is updated and resized
}

qreal FaceInvadersScene::getInvaderDeathLine()
{
    return m_gameSize.height();
}

FaceInvadersScene::GameState FaceInvadersScene::getState() const
{
    return m_gameState;
}

int FaceInvadersScene::getGameScore() const
{
    return m_gameScore;
}

void FaceInvadersScene::createNewInvaders()
{
    if(m_gameState != Playing)
        return;

    int newInvaders = rand()%2;
    for(int i = 0; i < newInvaders; i++)
    {
        Invader *invader = new Invader();
        this->addItem(invader);
        connect(invader, SIGNAL(AlienDying(int)), this, SLOT(alienEvaded(int)));

        invader->setPos(rand()%(int)m_gameSize.width(), -64*m_invaderScale);
        invader->setScale(0.65f + (rand()%35)/100.0f);
    }
    QTimer::singleShot(400+(rand()%700), this, SLOT(createNewInvaders()));
}

void FaceInvadersScene::updatePlayerPosition(QPoint position)
{
    this->player->setPos(position.x()*m_gameSize.width()/100,
                         m_playerStartingPosition.y());
}

void FaceInvadersScene::resetGame()
{
    QList<QGraphicsItem*> items = this->items();
    foreach(QGraphicsItem* item, items)
    {
        if(item != player && item != m_scoreItem)
        {
            this->removeItem(item);
            delete item;
        }
    }

    m_gameScore = 0;
    m_gameState = InitState;
    alienEvaded(0);
}

void FaceInvadersScene::pauseGame()
{
    if(m_gameState == Paused)
        return;

    m_gameState = Paused;
    m_advanceTimer->stop();
}

void FaceInvadersScene::endGame()
{
    if(m_gameState == Stopped)
        return;

    m_gameState = Stopped;
    m_advanceTimer->stop();
    emit gameOver(m_gameScore);
}

void FaceInvadersScene::beginGame()
{
    if(m_gameState == Playing)
        return;

    m_gameState = Playing;
    m_advanceTimer->start(1000/18);
    createNewInvaders();
    emit gameStarted(false);
}

void FaceInvadersScene::increaseInvaderSpeed()
{
}

void FaceInvadersScene::decreaseInvaderSpeed()
{
}

void FaceInvadersScene::setInvaderSpeed(qreal speed)
{
}

void FaceInvadersScene::alienEvaded(int points)
{
    m_gameScore += points;

    m_scoreItem->setText(QString("Score: %1").arg(m_gameScore,5,10,QChar('0')));
    updateScorePosition();
}

QImage *FaceInvadersScene::getPlayerImage()
{
    return player->getFace();
}

void FaceInvadersScene::setPlayerImage(QImage *image)
{
    player->setFace(image);
}

void FaceInvadersScene::drawBackground(QPainter *painter, const QRectF &rect)
{
    painter->fillRect(rect, Qt::black);
    painter->drawImage(QRect(0,-5,600,450), *background, background->rect());
}

void FaceInvadersScene::updateScorePosition()
{
    m_scoreItem->setPos(m_gameSize.width()-m_scoreItem->boundingRect().width()-10, 0);
}

FaceInvadersWidget::FaceInvadersWidget(QWidget *parent) :
    QGraphicsView(parent), m_scene(new FaceInvadersScene(this)),
    m_secondsBetweenGagmes(FaceInvadersWidget::DefaultTimeBetweenGames),
    m_initScreenSeconds(FaceInvadersWidget::DefaultInitScreenTime)
{
    this->setScene(m_scene);
    this->setRenderHint(QPainter::Antialiasing);
    this->setViewport(new QGLWidget(QGLFormat(QGL::SampleBuffers)));

    this->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

    connect(m_scene, SIGNAL(gameOver(int)), this, SIGNAL(gameOver(int)));
    connect(m_scene, SIGNAL(gamePaused()), this, SIGNAL(gamePaused()));
    connect(m_scene, SIGNAL(gameStarted(bool)), this, SIGNAL(gameStarted(bool)));

    //For Displaying GameOver message
    connect(m_scene, SIGNAL(gameOver(int)), this, SLOT(update()));
    connect(m_scene, SIGNAL(gameOver(int)), this, SLOT(m_gameOver(int)));
}

int FaceInvadersWidget::heightForWidth(int w) const
{
    return w;
}


void FaceInvadersWidget::resizeEvent(QResizeEvent *)
{
    m_scene->setSceneRect(0, 0, 600, 440);
    this->fitInView(0, 0, 600, 440, Qt::KeepAspectRatio);
}

void FaceInvadersWidget::drawForeground(QPainter *painter, const QRectF &rect)
{
    if(m_initScreen)
    {
        QRectF gameRect = m_scene->getGameScreenSize();
        QRectF rect2(gameRect.width()/4, gameRect.height()/4,
                    gameRect.width()/2, gameRect.height()/2);
        QPen pen(Qt::black);
        pen.setWidth(3);
        painter->setPen(pen);
        QBrush brush(QColor(0,0,0,196));
        painter->setBrush(brush);
        painter->drawRoundedRect(rect2, 8, 8);

        QRectF textRect(rect2.width()/8+rect2.x(),
                        rect2.height()/16+rect2.y(),
                        3*rect2.width()/4,
                        rect2.width()/8);

        QFont f("Helvetica", 20, QFont::Bold);
        pen.setColor(QColor(255,255,255,230));
        painter->setPen(pen);
        painter->setFont(f);
        painter->drawText(textRect, QString("Smile!"), QTextOption(Qt::AlignCenter));

        textRect.adjust(-rect2.width()/16, rect2.height()/6,
                        rect2.width()/8, rect2.height()/4);
        f.setPointSize(8);
        painter->setFont(f);
        pen.setColor(QColor(0xFA, 0xED, 0x57));
        painter->setPen(pen);
        painter->drawText(textRect, QString("At the end of the timer, your image will be captured."), QTextOption(Qt::AlignLeft));

        QImage *playerImage = m_scene->getPlayerImage();
        QPointF imagePoint(rect2.x() + rect2.width()/2-playerImage->width()/2,
                           textRect.y()+rect2.height()/6);
        painter->drawImage(imagePoint, *playerImage);
        painter->drawRect(QRectF(imagePoint, playerImage->size()));

        textRect.setY(imagePoint.y() + playerImage->height() + rect2.height()/16);
        textRect.setHeight(rect2.height()/8);

        painter->drawText(textRect, QString("Capturing image in %1....").arg(m_initScreenCountDown), QTextOption(Qt::AlignCenter));

    }
    else if(m_scene->getState() != FaceInvadersScene::Playing)
    {
        QRectF gameRect = m_scene->getGameScreenSize();
        QRectF rect2(gameRect.width()/4, gameRect.height()/4,
                    gameRect.width()/2, gameRect.height()/2);
        QPen pen(Qt::black);
        pen.setWidth(3);
        painter->setPen(pen);
        QBrush brush(QColor(0,0,0,196));
        painter->setBrush(brush);
        painter->drawRoundedRect(rect2, 8, 8);

        QRectF textRect(rect2.width()/8+rect2.x(),
                        rect2.height()/16+rect2.y(),
                        3*rect2.width()/4,
                        rect2.width()/8);

        QFont f("Helvetica", 20, QFont::Bold);
        pen.setColor(QColor(255,255,255,230));
        painter->setPen(pen);
        painter->setFont(f);
        painter->drawText(textRect, QString("Game Over"), QTextOption(Qt::AlignCenter));

        textRect.adjust(0,rect2.height()/4,0,rect2.height()/4);
        f.setPointSize(12);
        painter->setFont(f);
        pen.setColor(QColor(0xFA, 0xED, 0x57));
        painter->setPen(pen);
        painter->drawText(textRect, QString("Your Score: %1").arg(m_scene->getGameScore(),5,10,QChar('0')), QTextOption(Qt::AlignLeft));


        pen.setColor(QColor(0x82, 0xC1, 0xE8));
        painter->setPen(pen);
        textRect.adjust(0, rect2.height()/8, 0, rect2.height()/8);
        //! \todo Use QSettings to store highest score
        painter->drawText(textRect, QString("Highest Score: %1").arg(100), QTextOption(Qt::AlignLeft));

        f.setPointSize(10);
        pen.setColor(Qt::white);
        painter->setFont(f);
        painter->setPen(pen);
        textRect.adjust(0, rect2.height()/4, 0, rect2.height()/4);
        painter->drawText(textRect, QString("NEW game begins in %1...").arg(m_secondsToRestart), QTextOption(Qt::AlignLeft));
    }
}

void FaceInvadersWidget::m_gameOver(int score)
{
    //Begin timer for game reset
    QTimer::singleShot(1000, this, SLOT(resetTimerExpired()));
    m_secondsToRestart = 8;
}

void FaceInvadersWidget::resetTimerExpired()
{
    m_secondsToRestart--;
    this->viewport()->update();
    if(m_secondsToRestart == 0)
    {
        //Start new game
        FaceInvadersScene *scene = static_cast<FaceInvadersScene*>(this->scene());
        scene->resetGame();
        scene->beginGame();
    }
    else
        QTimer::singleShot(1000, this, SLOT(resetTimerExpired()));
}

void FaceInvadersWidget::initTimerExpired()
{
    m_initScreenCountDown--;
    this->viewport()->update();
    if(m_initScreenCountDown == 0)
    {
        //Start game
        this->beginGame();
        m_initScreen = false;
        emit ceaseImageUpdates();
    }
    else
        QTimer::singleShot(1000, this, SLOT(initTimerExpired()));
}

void FaceInvadersWidget::resetGame()
{
}

void FaceInvadersWidget::pauseGame()
{
}

void FaceInvadersWidget::endGame()
{

}

void FaceInvadersWidget::beginGame()
{
    m_scene->beginGame();
}

void FaceInvadersWidget::updatePlayerPosition(QPoint position)
{
    this->m_scene->updatePlayerPosition(position);
}

void FaceInvadersWidget::initScreen()
{
    if(m_initScreen)
    {
        m_initScreenCountDown = m_initScreenSeconds;
        return;
    }

    m_initScreen = true;
    m_initScreenCountDown = m_initScreenSeconds;
    m_scene->resetGame();
    this->viewport()->update();

    QTimer::singleShot(1000, this, SLOT(initTimerExpired()));

    emit faceImageUpdatesRequest();
}

void FaceInvadersWidget::updatePlayerImage(QImage *image)
{
    m_scene->setPlayerImage(image);
}

PlayerItem::PlayerItem(QGraphicsItem *parent, QGraphicsScene *scene) :
    QGraphicsItem(parent, scene)
{
    face = new QImage(":/images/defaultUser.png");
}

PlayerItem::~PlayerItem()
{
    delete face;
}

QRectF PlayerItem::boundingRect() const
{
    return QRectF(-32,-32,64,64);
}

QPainterPath PlayerItem::shape() const
{
    QPainterPath path;
    path.addRect(-32,-32,64,64);
    return path;
}

void PlayerItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    painter->drawImage(QPoint(-32,-32), *face, QRect(0,0,64,64));
    if(hit)
    {
        painter->setPen(Qt::red);
        painter->drawRect(-32,-32,64,64);
    }
}

void PlayerItem::setFace(QImage *image)
{
    if(image == NULL)
        return;

    delete face;
    face = image;
}

QImage *PlayerItem::getFace()
{
    return face;
}

void PlayerItem::advance(int phase)
{
    QList<QGraphicsItem*> colliding = this->collidingItems();
    hit = (colliding.size() > 0);
    if(colliding.size() > 0)
    {
        emit PlayerHit();
    }
}

Invader::Invader(QGraphicsItem *parent, QGraphicsScene *scene) :
    QGraphicsItem(parent, scene)
{
    qsrand(QTime::currentTime().msec());
    m_type = (InvaderType)(rand()%InvaderTypeCount);
    m_fallVelocity = (qreal)(rand()%1000)/100+3;
    m_angularVelocity = (qreal)(rand()%1415)/1000;
    m_pointValue = rand()%10;

    switch(m_type)
    {
    case Apple:
        initApple();
        break;
    case Banana:
        initBanana();
        break;
    case Watermelon:
        initWatermelon();
        break;
    case Bug:
        initBug();
        break;
    default:
        /* Should never happen, just default to Apple I guess */
        m_type = Apple;
        initApple();
        break;
    }
}

Invader::~Invader()
{
    delete m_image;
}

QRectF Invader::boundingRect() const
{
    return m_boundingRect;
}

QPainterPath Invader::shape() const
{
    return m_shape;
}

void Invader::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    painter->drawImage(m_boundingRect.topLeft(), *m_image, m_image->rect());
#ifdef DEBUG_INVADER_SHAPE
    painter->drawPath(m_shape);
#endif
}

void Invader::advance(int phase)
{
    if(phase == 0)
        return;

    qreal deathLine = static_cast<FaceInvadersScene*>(this->scene())->getInvaderDeathLine();
    if(this->pos().y() > deathLine)
    {
        this->scene()->removeItem(this);
        emit AlienDying(m_pointValue);
        delete this;
        return;
    }
    setPos(mapToParent(0, m_fallVelocity));
}

void Invader::initApple()
{
    m_image = new QImage(":/images/invaders/AppleInvader.png");
    m_boundingRect = QRectF(-32,-32,64,64);
    m_shape.addEllipse(-23,-20,50,50);
}

void Invader::initBanana()
{
    m_image = new QImage(":/images/invaders/BananaInvader.png");
    m_boundingRect = QRectF(-32,-32,64,64);
    m_shape.addRect(-26,-16,58,30);
}

void Invader::initWatermelon()
{
    m_image = new QImage(":/images/invaders/WatermelonInvader.png");
    m_boundingRect = QRectF(-32,-32,64,64);
    m_shape.addEllipse(-30,-30,60,60);
}

void Invader::initBug()
{
    m_image = new QImage(":/images/invaders/BugInvader.png");
    m_boundingRect = QRectF(-32,-32,64,64);
    m_shape.addRect(-32,-32,64,64);
}
