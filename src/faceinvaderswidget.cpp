#include "faceinvaderswidget.h"
#include <QLinearGradient>
#include <QDebug>
#include <QGraphicsEllipseItem>
#include <QTimer>
#include <QTime>

FaceInvadersScene::FaceInvadersScene(QWidget *parent):
    QGraphicsScene(parent)
{
    this->setBackgroundBrush(QBrush(Qt::black));

    background = new QImage(QString(":/images/background2.png"));

    player = new PlayerItem();
    this->addItem(player);

    player->setPos(280, 350);
}

FaceInvadersScene::~FaceInvadersScene()
{
    delete background;
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
    m_type = (InvaderType)(qrand()%InvaderTypeCount);
    m_fallVelocity = (qreal)(qrand()%1000)/100;
    m_angularVelocity = (qreal)(qrand()%1415)/1000;

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
}

QRectF Invader::boundingRect() const
{
}

QPainterPath Invader::shape() const
{
}

void Invader::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
}

void Invader::advance(int phase)
{
}


void Invader::initApple()
{
}

void Invader::initBanana()
{
}

void Invader::initWatermelon()
{
}

void Invader::initBug()
{
}
