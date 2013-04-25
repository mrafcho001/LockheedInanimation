/*! \file       faceinvaderswidget.h
    \author     Martin Bakiev
    \date       March, 2013
    \version    1.0
    \brief      This file contiains the classes for the logic and display of
                the Face Invaders game.
                FaceInvadersScene is the object which contains all of the game
                logic and takes care of visual object animation and collision
                detection.
                FaceInvadersWidget is an UI widget manifistation of the FaceInvadersScene
                object. This can be used to display the game to the user.
                PlayerItem represents and manages the player object and is
                responsible for detecting collisions with Invader Objects.
                Invader represents an 'invader' object falling from the skies, which the
                player attempts to avoid.

    \sa FaceInvadersScene, FaceInvadersWidget, PlayerItem, Invader
*/

#ifndef FACEINVADERSWIDGET_H
#define FACEINVADERSWIDGET_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QGraphicsSimpleTextItem>
#include <QPoint>
#include <QPixmap>
#include <QSharedPointer>
#include <QTimer>


typedef QSharedPointer<QImage> QImageSharedPointer;
Q_DECLARE_METATYPE(QImageSharedPointer);

//Forward declaration
class PlayerItem;


/*! \brief Manages game logic and graphical scene.

  This object is responsible for managing all of the players and invaders on the
  graphical scene, as well as drawing the background. The graphical scene is used
  This object emits signals to notify of game state changes.

  The game can be paused, resumed, ended, and reset.
  \sa FaceInvadersScene::pauseGame()
  \sa FaceInvadersScene::beginGame()
  \sa FaceInvadersScene::endGame()
  \sa FaceInvadersScene::resetGame()

  <b>Game State Transitioning</b>
  To transition between, what is normally thought of "Paused", "Stopped", and "Playing"
  states, the class provides several functions. FaceInvadersScene::beginGame() can be
  used to go into "Playing" mode without changing the current state of the scene.
  This allows flexibility for setting up the scene as desired and beginning the game
  from there. This function makes the existance of a "resume()" function unnecessary.
  FaceInvadersScene::resetGame() resets the scene state to an clear state having
  no Invaders, and a 0 score. FaceInvadersScene::endGame() forces the game to stop
  and display a game end message, whereas FaceInvadersScene::pauseGame() stops the
  game and displays a paused game message.

*/
class FaceInvadersScene : public QGraphicsScene
{
    Q_OBJECT
public:
    enum GameState { Playing, Paused, Stopped, InitState };
    /*! \brief Default constructor
        \param parent Sets the parent widget, \seeqtdoc
    */
    explicit FaceInvadersScene(QWidget *parent = 0);
    //! \brief Destructor
    ~FaceInvadersScene();


    //! \brief Returns rectangle of the game space.
    QRectF getGameScreenSize();
    //! \brief Sets the rectangle for the game space.
    void setGameScreenSize(QRectF &rect);

    //! \brief Sets the line past which all invaders are destroyed
    qreal getInvaderDeathLine();

    //! \brief Returns the current state of the game
    GameState getState() const;

    //! \brief Returns the score of the game
    int getGameScore() const;

    int getHighScore() const;

signals:
    /*! \brief Idicates game state transition to <i>game over</i>

      <i>Game over</i> is defined as the player failure to avoid falling invaders
      or explicit call to FaceInvadersScene::endGame(). This signal is not
      emitted when the game is paused.
      \sa FaceInvadersScene::gamePaused()
      \param score The score of the game that just ended
    */
    void gameOver(int m_scoreItem);

    /*! \brief Indiccates game state transition from <i>playing</i> to <i>paused</i>

      The game has been explicitly paused by a call to FaceInvadersScene::pauseGame()
      \sa FaceInvadersScene::pauseGame()
    */
    void gamePaused();

    /*! \brief Indicates game state transition from <i>stopped</i> to <i>playing</i>
        \param resumed Indicates if game was resumed, or fresh game was started
    */
    void gameStarted(bool resumed);

public slots:
    /*! \brief  Creates random number of Invaders, positioned randomly throughout
                the game screen.
    */
    void createNewInvaders();

    //! \brief Updates the position of the player (only x-position!)
    void updatePlayerPosition(QPoint position);

    //! \brief Transitions the game into <i>initial</i> state
    void resetGame();

    //! \brief Transitions the game into <i>paused</i> state
    void pauseGame();

    //! \brief Transitions the game into <i>stopped</i> state
    void endGame();

    //! \brief Transitions the game tion <i>playing</i> state
    void beginGame();

    //! \brief Increase the falling velocity of invaders
    void increaseInvaderSpeed();

    //! \brief Decrease the falling velocity of invaders
    void decreaseInvaderSpeed();

    /*! \brief Set invader speed to specific value
      \todo explain invader speed value...
      \param speed ??
    */
    void setInvaderSpeed(qreal speed);

    //! \brief Invader has died, award points to player
    void alienEvaded(int points);

    //! \brief Get Player image
    QPixmap *getPlayerImage();

    //! \brief Sets the player image
    void setPlayerImage(const QPixmap &image);

    void setHighScore(int score);

protected:
    //! \brief Draws background for the game
    void drawBackground(QPainter *painter, const QRectF &rect);

private:
    //! \brief Makes sure the score is always visible and does not run off the viewable area.
    void updateScorePosition();


    QImage *background; //!< Image to be painted as the background
    PlayerItem *player; //!< PlayerItem for easy access
    QGraphicsSimpleTextItem *m_scoreItem;  //!< Displays game score

    GameState m_gameState;   //!< 1 - Game running, 0 - Game not running
    int m_gameScore;         //!< Maintains the game score
    int m_highScore;

    //Game Parameters
    //! The starting position of player, Y-coordinate is maintained throughout game play
    QPointF m_playerStartingPosition;

    //! The size of the graphics scene, larger values result in more detailed graphics and smaller invaders
    QRectF m_gameSize;

    //! The scaling of the invaders, <1 -> smaller, >1 -> larger, 1 -> unscaled
    qreal m_invaderScale;
    //! Invader speed parameter
    qreal m_invaderSpeed;

    //Timers
    QTimer *m_advanceTimer;     //!< Timer for controlling Invader descent


public:
    static const QRect DefaultGameSize;
};

/*! \brief UI class for displaying the FaceInvaders game

  This class can be used to display the FaceInvaders game to the user. The class
  provides similar signals and slots as the FaceInvadersScene class and behaves
  more or less as a shell around it. As it is only used for Displaying purposes, it
  does not contribute to the game logic.
*/
class FaceInvadersWidget : public QGraphicsView
{
    Q_OBJECT
public:
    /*! \brief Constructor
        \param parent Sets the parent Widget, \seeqtdoc
    */
    explicit FaceInvadersWidget(QWidget *parent = 0);
    ~FaceInvadersWidget();

    //! \brief Used to maintain aspect ratio of widget, \seeqtdoc
    int heightForWidth(int w) const;

signals:
    //! \brief See FaceInvadersScene::gameOver()
    void gameOver(int score);

    //! \brief See FaceInvadersScene::gamePaused()
    void gamePaused();

    /*! \brief Indicates game state transition from <i>stopped</i> to <i>playing</i>
        \param resumed Indicates if game was resumed, or fresh game was started
    */
    void gameStarted(bool resumed);

    //! \brief Indicates that the class wants to recieve updates for the player iamge
    void faceImageUpdatesRequest();

    /*! \brief Indicates that the class no longer wants to recieve updates for
               the player image. */
    void ceaseImageUpdates();

public slots:
    //! \brief Calls FaceInvadersScene::resetGame()
    void resetGame();

    //! \brief Calls FaceInvadersScene::pauseGame()
    void pauseGame();

    //! \brief Calls FaceInvadersScene::endGame()
    void endGame();

    //! \brief Calls FaceInvadersScene::beginGame()
    void beginGame();

    //! \brief Calls FaceInvadersScene::updatePlayerPosition()
    void updatePlayerPosition(QPoint position);
    //! \brief Calls FaceInvadersScene::updatePlayerPosition()
    void updatePlayerPosition(QRect position);

    /*! \brief Shows a brief screen to acquire user image

        This function emits RequestFaceImageUpdates, and
        expects the owner to supply images through the
    */
    void initScreen();

    //! \brief Sets the player image
    void updatePlayerImage(QImageSharedPointer image);

protected:
    //! \brief Used to maintain aspect ratio, \seeqtdoc
    void resizeEvent(QResizeEvent *);

    //! \brief Displays Game Over message
    void drawForeground(QPainter *painter, const QRectF &rect);
protected slots:
    //! \brief Initializes countdown timer for new game
    void m_gameOver(int score);

    //! \brief Updates countdown for starting new game
    void resetTimerExpired();

    //! \brief Updates countdown for user face image acquisition
    void initTimerExpired();

private:
    FaceInvadersScene *m_scene; //!< The game graphics scene

    int m_secondsToRestart;     //!< Maintains count down for starting new game

    bool m_initScreen;
    int m_initScreenCountDown;


    //Tunable variables
    int m_secondsBetweenGagmes;
    int m_initScreenSeconds;

    QTimer m_initTimer;
    QTimer m_restartTimer;

public:
    static const int DefaultInitScreenTime = 10;
    static const int DefaultTimeBetweenGames = 10;
};

/*! \brief  This is the graphics item representing the user in the
            graphics scene.

    This class is responsbile for displaying the user image and provid the
    collision detection with Invaders to determine when the game is over.
*/
class PlayerItem : public QObject, public QGraphicsItem
{
    Q_OBJECT
public:
    //! \brief Constructor
    explicit PlayerItem(QGraphicsItem *parent = 0, QGraphicsScene *scene = 0);

    //! \brief Destructor
    ~PlayerItem();

    //! \brief Bounding rectangle of the drawing area, \seeqtdoc
    QRectF boundingRect() const;

    //! \brief Accurate representation of the item, \seeqtdoc
    QPainterPath shape() const;

    //! \brief Drawing of the item, \seeqtdoc
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    /*! \brief Sets the image for the user's face.
      \warning The image is not copied! This class takes ownership of the pointer
      and frees the memory when done.
    */
    void setFace(const QPixmap &image);

    //! \brief Gets the face image of the player
    QPixmap *getFace();

signals:
    //! \brief Indicates the player has collided with an Invader
    void PlayerHit();

protected:
    //! \brief Used for collision detection
    void advance(int phase);

private:
    QPixmap *m_face;   //!< User face image

    bool hit; //Tempurary variable...

};


class Invader : public QObject, public QGraphicsItem
{
    Q_OBJECT
public:
    //! Identifies the type of invader
    enum InvaderType { Apple = 0, Banana, Watermelon, Bug, InvaderTypeCount };

    //! \brief Default Constructor
    explicit Invader(QGraphicsItem *parent = 0, QGraphicsScene *scene = 0);
    //! \brief Destructor
    ~Invader();

    //! \brief Defines the drawing area of the invader
    QRectF boundingRect() const;
    //! \brief Defines the collision detection shape of the invader
    QPainterPath shape() const;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    InvaderType getType() const;
    int absorbReward();

signals:
    //! \brief Notify scene of player successfully evading the invader
    void AlienDying(int points);

protected:
    //! \brief Used for animation
    void advance(int phase);

private:
    InvaderType m_type;         //!< Indicates the type of invader represented
    QRectF m_boundingRect;      //!< The bounding rectangle of the invader
    QPainterPath m_shape;       //!< The shape of the invader
    QPixmap *m_image;            //!< Image of the invader

    //Invader falling parameters
    qreal m_fallVelocity;       //!< The rate of falling from top to bottom of the Invader
    qreal m_angularVelocity;    //!< The rate of rotation of the Invader

    int m_pointValue;           //!< The points awarded to player to evading


    void initApple();
    void initBanana();
    void initWatermelon();
    void initBug();

    /*! \todo Use function pointers instead of switch statement
        Idea on hold...
    typedef void (Invader::*MemberFuncPointer)();
    static const MemberFuncPointer initFuncs[] = { &Invader::initApple, &Invader::initBanana };
    */
};

#endif // FACEINVADERSWIDGET_H































