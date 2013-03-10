#ifndef FACEINVADERSWIDGET_H
#define FACEINVADERSWIDGET_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QPoint>
#include <QImage>

class PlayerItem;


class FaceInvadersScene : public QGraphicsScene
{
    Q_OBJECT
public:
    explicit FaceInvadersScene(QWidget *parent = 0);
    ~FaceInvadersScene();

signals:

public slots:
    void createNewInvaders();
    void updatePlayerPosition(QPoint position);

protected:
    void drawBackground(QPainter *painter, const QRectF &rect);

private:
    QImage *background;
    PlayerItem *player;


    //Timers
    QTimer *m_advanceTimer;
    QTimer *m_summonerTimer;
};

class FaceInvadersWidget : public QGraphicsView
{
    Q_OBJECT
public:
    explicit FaceInvadersWidget(QWidget *parent = 0);
    int heightForWidth(int w) const;
    
signals:
    void gameOver(int score);
    void gamePaused();
    
public slots:
    void resetGame();
    void pauseGame();

    void updatePlayerPosition(QPoint position);

protected:
    void resizeEvent(QResizeEvent *);

private:
    FaceInvadersScene *m_scene;

};

class PlayerItem : public QGraphicsItem
{
public:
    explicit PlayerItem(QGraphicsItem *parent = 0, QGraphicsScene *scene = 0);
    ~PlayerItem();

    QRectF boundingRect() const;
    QPainterPath shape() const;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    void setFace(QImage *image);

protected:

    void advance(int phase);

private:
    QImage *face;

};


class Invader : public QGraphicsItem
{
public:
    enum InvaderType { Apple = 0, Banana, Watermelon, Bug, InvaderTypeCount };

    explicit Invader(QGraphicsItem *parent = 0, QGraphicsScene *scene = 0);
    ~Invader();

    QRectF boundingRect() const;
    QPainterPath shape() const;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

protected:
    void advance(int phase);

private:
    InvaderType m_type;         //!< Indicates the type of invader represented
    QRectF m_boundingRect;      //!< The bounding rectangle of the invader
    QPainterPath m_shape;       //!< The shape of the invader
    QImage *m_image;

    //Invader falling parameters
    qreal m_fallVelocity;       //!< The rate of falling from top to bottom of the Invader
    qreal m_angularVelocity;    //!< The rate of rotation of the Invader



    void initApple();
    void initBanana();
    void initWatermelon();
    void initBug();

    /*! \Todo Use function pointers instead of switch statement
        Idea on hold...
    typedef void (Invader::*MemberFuncPointer)();
    static const MemberFuncPointer initFuncs[] = { &Invader::initApple, &Invader::initBanana };
    */
};

#endif // FACEINVADERSWIDGET_H































