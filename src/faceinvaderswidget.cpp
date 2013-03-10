#include "faceinvaderswidget.h"
#include <QLinearGradient>
#include <QDebug>
#include <QGraphicsEllipseItem>
#include <QTimer>
#include <QTime>
#include <QThread>
#include <cstdlib>

FaceInvadersScene::FaceInvadersScene(QWidget *parent):
    QGraphicsScene(parent)
{
    this->setBackgroundBrush(QBrush(Qt::black));

    background = new QImage(QString(":/images/background2.png"));

    player = new PlayerItem();
    this->addItem(player);

    player->setPos(280, 350);

    m_advanceTimer = new QTimer(this);

    connect(m_advanceTimer, SIGNAL(timeout()), this, SLOT(advance()));

    m_advanceTimer->start(1000/18);
    int seed = QTime::currentTime().msec();
    seed *= QTime::currentTime().second();
    seed *= QTime::currentTime().minute();
    srand(QThread::currentThreadId() * seed);
    createNewInvaders();
}

FaceInvadersScene::~FaceInvadersScene()
{
    delete background;
}

void FaceInvadersScene::createNewInvaders()
{
    int newInvaders = rand()%2;
    for(int i = 0; i < newInvaders; i++)
    {
        Invader *invader = new Invader();
        this->addItem(invader);

        invader->setPos(rand()%600, -64);
    }
    QTimer::singleShot(300+(rand()%700), this, SLOT(createNewInvaders()));
}

void FaceInvadersScene::updatePlayerPosition(QPoint position)
{
    this->player->setPos(position.x()*6, 380);
}

void FaceInvadersScene::drawBackground(QPainter *painter, const QRectF &rect)
{
    painter->fillRect(rect, Qt::black);
    painter->drawImage(QRect(0,-5,600,450), *background, background->rect());
}

FaceInvadersWidget::FaceInvadersWidget(QWidget *parent) :
    QGraphicsView(parent)
{
    m_scene = new FaceInvadersScene(this);
    this->setScene(m_scene);
    this->setRenderHint(QPainter::Antialiasing);

    QPen pen(Qt::black);
    QBrush brush(Qt::blue);
    QGraphicsEllipseItem *ellipse = m_scene->addEllipse(0,0,30,30, pen, brush);

    QFont font;
    font.setPointSize(12);
    QGraphicsTextItem *text1 = m_scene->addText("50,50",font);
    QGraphicsTextItem *text2 = m_scene->addText("450,450",font);
    text1->setPos(50,50);
    text2->setPos(400,400);

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


void FaceInvadersWidget::resetGame()
{
}

void FaceInvadersWidget::pauseGame()
{
}

void FaceInvadersWidget::updatePlayerPosition(QPoint position)
{
    this->m_scene->updatePlayerPosition(position);
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
}

void PlayerItem::setFace(QImage *image)
{
    face = image;
}

void PlayerItem::advance(int phase)
{
}


Invader::Invader(QGraphicsItem *parent, QGraphicsScene *scene) :
    QGraphicsItem(parent, scene)
{
    qsrand(QTime::currentTime().msec());
    m_type = (InvaderType)(rand()%InvaderTypeCount);
    m_fallVelocity = (qreal)(rand()%1000)/100+3;
    m_angularVelocity = (qreal)(rand()%1415)/1000;

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
}

void Invader::advance(int phase)
{
    if(phase == 0)
        return;

    if(this->pos().y() > 300)
    {
        this->scene()->removeItem(this);
        delete this;
        return;
    }
    setPos(mapToParent(0, m_fallVelocity));
}

void Invader::initApple()
{
    m_image = new QImage(":/images/invaders/AppleInvader.png");
    m_boundingRect = QRectF(-32,-32,64,64);
    m_shape.addEllipse(-25,-25,50,50);
}

void Invader::initBanana()
{
    m_image = new QImage(":/images/invaders/BananaInvader.png");
    m_boundingRect = QRectF(-32,-32,64,64);
    m_shape.addRect(-26,16,58,30);
}

void Invader::initWatermelon()
{
    m_image = new QImage(":/images/invaders/WatermelonInvader.png");
    m_boundingRect = QRectF(-32,-32,64,64);
    m_shape.addEllipse(-32,-32,64,64);
}

void Invader::initBug()
{
    m_image = new QImage(":/images/invaders/BugInvader.png");
    m_boundingRect = QRectF(-32,-32,64,64);
    m_shape.addRect(-32,-32,64,64);
}
