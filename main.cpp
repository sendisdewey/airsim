#include <QApplication>
#include <QWidget>
#include <QPainter>
#include <QTimer>
#include <QKeyEvent>
#include <QVector2D>
#include <QPixmap>
#include <QVector>
#include <cmath>
#include <QFile>
#include <QTextStream>
#include <QFont>
#include <QDir>

// структуры данных | data structures
struct Target {
    float x, y;
    bool alive;
    int type;  // 0 - танк, 2 - зенитное орудие | 0 - tank, 2 - AA
    int hp;
    float angle;

    Target(float x_, float y_, int t) : x(x_), y(y_), alive(true), type(t), hp(3), angle(0) {}
};

struct HitMarker {
    float x, y;
    int timer;  // сколько кадров осталось | frames left
};

struct AABullet {
    float x, y;
    float vx, vy;
    int timer;
};

struct Airfield {
    float x, y;
    Airfield(float x_, float y_) : x(x_), y(y_) {}
};

struct GrassBlade {
    int x, y;
    int length;
    int angle;
};

struct EnemyPlane {
    float x, y;
    float vx, vy;
    float angle;
    int hp;
    int type;  // 0 - Eurofighter, 1 - F-18
    bool alive;
    int shootTimer;  // таймер для стрельбы | shooting cooldown

    EnemyPlane(float x_, float y_, int t)
        : x(x_), y(y_), vx(0), vy(0), angle(0), hp(3), type(t), alive(true), shootTimer(0) {}
};

struct Rocket {
    float x, y;
    float vx, vy;
    int timer;
    Target *target;
    EnemyPlane *enemyTarget;
    bool isHoming;
};


struct PlaneSkin {
    QString name;
    int price;
    int speedBonus;      // бонус к скорости | speed bonus
    int armorBonus;      // бонус к броне | armor bonus
    int bombBonus;       // доп. бомбы | extra bombs
    float turnSpeed;     // скорость поворота | turn speed
    bool owned;
    QString spriteFile;
    int flameOffsetX;    // смещение пламени по X
    int flameOffsetY1;   // смещение по Y для левого двигателя
    int flameOffsetY2;   // смещение по Y для правого двигателя
};

class AirplaneGame : public QWidget
{
public:
    AirplaneGame(QWidget *parent = nullptr)
        : QWidget(parent),
          posX(63850), posY(64010),
          velocity(0, 0),
          angle(0),
          thrust(0),
          fuel(100),
          bombs(50),
          money(0),
          armor(100),
          keyW(false), keyS(false), keyA(false), keyD(false),
          onGround(false),
          blink(false),
          fuelWarningPlayed(false),
          airfield(64000, 64000),
          shopOpen(false),
          selectedSkin(0),
          currentPlane(0),
          landingMenuOpen(false),
          landingChoice(0),
          hydraulicFailure(false),
          hydraulicTimer(0),
          forsageAlpha(0.0f)
    {
        resize(1920, 1000);
        setMinimumSize(800, 600);
        setWindowTitle("airsim");

        forsageSprite.load(":/forsage.png");
        if (!forsageSprite.isNull()) {
            forsageSprite = forsageSprite.scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }

        // загрузка спрайтов игрока
        su27Sprite.load(":/plane.png");
        if (!su27Sprite.isNull()) {
            su27Sprite = su27Sprite.scaled(90, 90, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }

        mig29Sprite.load(":/plane7.png");
        if (!mig29Sprite.isNull()) {
            mig29Sprite = mig29Sprite.scaled(110, 110, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }

        su30Sprite.load(":/plane1.png");
        if (!su30Sprite.isNull()) {
            su30Sprite = su30Sprite.scaled(90, 90, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }

        su33Sprite.load(":/plane6.png");
        if (!su33Sprite.isNull()) {
            su33Sprite = su33Sprite.scaled(90, 90, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }

        su25Sprite.load(":/plane4.png");
        if (!su25Sprite.isNull()) {
            su25Sprite = su25Sprite.scaled(85, 85, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }

        su34Sprite.load(":/plane2.png");
        if (!su34Sprite.isNull()) {
            su34Sprite = su34Sprite.scaled(130, 130, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }

        su57Sprite.load(":/plane3.png");
        if (!su57Sprite.isNull()) {
            su57Sprite = su57Sprite.scaled(90, 90, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }

        mig31Sprite.load(":/plane5.png");
        if (!mig31Sprite.isNull()) {
            mig31Sprite = mig31Sprite.scaled(110, 110, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }

        // загрузка вражеских спрайтов
        eurofighterSprite.load(":/EF.png");
        if (!eurofighterSprite.isNull()) {
            eurofighterSprite = eurofighterSprite.scaled(95, 95, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }

        f18Sprite.load(":/f18.png");
        if (!f18Sprite.isNull()) {
            f18Sprite = f18Sprite.scaled(80, 80, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }

        // текстуры окружения
        grassTexture.load(":/grass_texture.png");
        if (!grassTexture.isNull()) {
            grassTexture = grassTexture.scaled(128, 128, Qt::KeepAspectRatio);
        }

        asphaltTexture.load(":/asphalt.png");
        if (!asphaltTexture.isNull()) {
            asphaltTexture = asphaltTexture.scaled(128, 128, Qt::KeepAspectRatio);
        }

        tankSprite.load(":/tank.png");
        if (!tankSprite.isNull()) {
            tankSprite = tankSprite.scaled(50, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }

        AASprite.load(":/AA.png");
        if (!AASprite.isNull()) {
            AASprite = AASprite.scaled(50, 50, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }

        treeSprite.load(":/tree.png");
        if (!treeSprite.isNull()) {
            treeSprite = treeSprite.scaled(80, 80, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }

        rocketSprite.load(":/rocket.png");
        if (!rocketSprite.isNull()) {
            rocketSprite = rocketSprite.scaled(40, 30, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }

        // магазин
        skins.append({"СУ-27", 0, 2, 0, 0, 3.0f, true, ":/plane.png", -58, -5, 5});
        skins.append({"МиГ-29", 1000, 3, 10, 30, 3.0f, false, ":/plane7.png", -58, -5, 5});
        skins.append({"СУ-30", 2000, 3, 10, 30, 3.0f, false, ":/plane1.png", -61, -5, 5});
        skins.append({"СУ-33", 2500, 3, 15, 30, 3.5f, false, ":/plane6.png", -61, -5, 5});
        skins.append({"СУ-34", 3000, 3, 15, 40, 2.5f, false, ":/plane2.png", -75, -5, 5});
        skins.append({"СУ-25", 3500, 2, 25, 50, 2.5f, false, ":/plane4.png", -37, -5, 5});
        skins.append({"СУ-57", 5000, 5, 20, 60, 4.0f, false, ":/plane3.png", -55, -6, 6});
        skins.append({"МиГ-31", 10000, 10, 20, 70, 3.0f, false, ":/plane5.png", -75, -3, 3});

        loadMoney();
        loadHangar();

        // генерация мира
        for (int i = 0; i < 10; i++) {
            float x = 1000 + rand() % 126000;
            float y = 1000 + rand() % 126000;
            targets.append(Target(x, y, (rand() % 2 == 0) ? 0 : 2));
        }

        // вражеские самолёты
        for (int i = 0; i < 10; i++) {
            float x = 1000 + rand() % 126000;
            float y = 1000 + rand() % 126000;
            enemies.append(EnemyPlane(x, y, rand() % 2));
        }

        // деревья
        for (int i = 0; i < 1000; i++) {
            float x = 1000 + rand() % 126000;
            float y = 1000 + rand() % 126000;
            trees.append(QPointF(x, y));
        }

        // таймеры
        QTimer *timer = new QTimer(this);
        connect(timer, &QTimer::timeout, [this]() { updateGame(); });
        connect(timer, &QTimer::timeout, this, QOverload<>::of(&AirplaneGame::update));
        timer->start(20);

        QTimer *blinkTimer = new QTimer(this);
        connect(blinkTimer, &QTimer::timeout, [this]() { blink = !blink; });
        blinkTimer->start(500);

        setFocusPolicy(Qt::StrongFocus);
    }

    void saveHangar() {
        QFile file("hangar.txt");
        if (file.open(QIODevice::WriteOnly)) {
            QTextStream out(&file);
            for (int i = 0; i < skins.size(); i++) {
                out << (skins[i].owned ? 1 : 0) << " ";
            }
            out << "\n" << currentPlane;
            file.close();
        }
    }

    void loadHangar() {
        QFile file("hangar.txt");
        if (file.open(QIODevice::ReadOnly)) {
            QTextStream in(&file);
            for (int i = 0; i < skins.size(); i++) {
                int owned;
                in >> owned;
                skins[i].owned = (owned == 1);
            }
            in >> currentPlane;
            file.close();
        }
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // камера
        float camX = posX - width()/2;
        float camY = posY - height()/2;
        if (camX < 0) camX = 0;
        if (camY < 0) camY = 0;
        if (camX > 128000 - width()) camX = 128000 - width();
        if (camY > 128000 - height()) camY = 128000 - height();
        painter.translate(-camX, -camY);

        // фон
        if (!grassTexture.isNull()) {
            painter.setBrush(QBrush(grassTexture));
            painter.setPen(Qt::NoPen);
            painter.drawRect(0, 0, 128000, 128000);
        }

        // деревья
        if (!treeSprite.isNull()) {
            for (const QPointF &tree : trees) {
                painter.save();
                painter.translate(tree.x(), tree.y());
                QPointF center(treeSprite.width() / 2.0, treeSprite.height() / 2.0);
                painter.drawPixmap(-center, treeSprite);
                painter.restore();
            }
        }

        // вражеские самолёты
        for (const EnemyPlane &e : enemies) {
            if (!e.alive) continue;

            painter.save();
            painter.translate(e.x, e.y);
            painter.rotate(e.angle + 90);

            QPixmap *enemySprite = (e.type == 0) ? &eurofighterSprite : &f18Sprite;
            QPointF center(enemySprite->width() / 2.0, enemySprite->height() / 2.0);
            painter.drawPixmap(-center, *enemySprite);

            if (e.hp < 3) {
                painter.setPen(Qt::NoPen);
                painter.setBrush(Qt::red);
                painter.drawRect(-15, -25, 30, 4);
                painter.setBrush(Qt::green);
                painter.drawRect(-15, -25, 10 * e.hp, 4);
            }

            painter.restore();
        }

        // ракеты игрока
        if (!rocketSprite.isNull()) {
            for (const Rocket &r : rockets) {
                painter.save();
                painter.translate(r.x, r.y);
                float angle = atan2(r.vy, r.vx) * 90 / M_PI;
                painter.rotate(angle);
                QPointF center(rocketSprite.width() / 2.0, rocketSprite.height() / 2.0);
                painter.drawPixmap(-center, rocketSprite);
                painter.restore();
            }
        }

        // аэродром
        painter.save();
        painter.translate(airfield.x, airfield.y);

        if (!asphaltTexture.isNull()) {
            painter.setBrush(QBrush(asphaltTexture));
            painter.setPen(Qt::NoPen);
            painter.drawRect(-300, -30, 1200, 80);
        }

        if (fuel < 30 && blink && !onGround) {
            painter.setBrush(Qt::red);
        } else {
            painter.setBrush(Qt::yellow);
        }
        for (int i = -280; i <= 900; i += 45) {
            painter.drawEllipse(i, -45, 7, 7);
            painter.drawEllipse(i, 55, 7, 7);
        }

        painter.restore();

        // цели
        for (const Target &t : targets) {
            if (!t.alive) continue;
            painter.save();
            painter.translate(t.x, t.y);

            if (t.type == 0) {
                if (!tankSprite.isNull()) {
                    QPointF center(tankSprite.width() / 2.0, tankSprite.height() / 2.0);
                    painter.drawPixmap(-center, tankSprite);
                }
            }
            else {
                if (!AASprite.isNull()) {
                    QPointF center(AASprite.width() / 2.0, AASprite.height() / 2.0);
                    painter.drawPixmap(-center, AASprite);
                    painter.setPen(QPen(Qt::black, 3));
                    painter.drawLine(0, 0, cos(t.angle) * 25, sin(t.angle) * 25);
                }
            }
            painter.restore();

            if (t.type == 2 && t.hp < 3) {
                painter.setPen(Qt::NoPen);
                painter.setBrush(Qt::red);
                painter.drawRect(t.x - 12, t.y - 25, 24, 4);
                painter.setBrush(Qt::green);
                painter.drawRect(t.x - 12, t.y - 25, 8 * t.hp, 4);
            }
            if (t.type == 0 && t.hp < 3) {
                painter.setPen(Qt::NoPen);
                painter.setBrush(Qt::red);
                painter.drawRect(t.x - 12, t.y - 20, 24, 4);
                painter.setBrush(Qt::green);
                painter.drawRect(t.x - 12, t.y - 20, 8 * t.hp, 4);
            }
        }

        // пули зениток
        painter.setBrush(Qt::yellow);
        painter.setPen(Qt::NoPen);
        for (const AABullet &b : AABullets) {
            painter.drawEllipse(b.x - 2, b.y - 2, 4, 4);
        }

        // маркеры попаданий
        for (const HitMarker &h : hitMarkers) {
            float progress = h.timer / 10.0;
            painter.setPen(QPen(QColor(255, 100, 0, 255 * progress), 2));
            painter.drawLine(h.x - 8, h.y - 8, h.x + 8, h.y + 8);
            painter.drawLine(h.x + 8, h.y - 8, h.x - 8, h.y + 8);
        }

        // самолёт игрока
        painter.save();
        painter.translate(posX, posY);
        painter.rotate(angle + 90);

        QPixmap *currentSprite = &su27Sprite;

        if (currentPlane == 1 && !mig29Sprite.isNull()) {
            currentSprite = &mig29Sprite;
        }
        else if (currentPlane == 2 && !su30Sprite.isNull()) {
            currentSprite = &su30Sprite;
        }
        else if (currentPlane == 3 && !su33Sprite.isNull()) {
            currentSprite = &su33Sprite;
        }
        else if (currentPlane == 4 && !su34Sprite.isNull()) {
            currentSprite = &su34Sprite;
        }
        else if (currentPlane == 5 && !su25Sprite.isNull()) {
            currentSprite = &su25Sprite;
        }
        else if (currentPlane == 6 && !su57Sprite.isNull()) {
            currentSprite = &su57Sprite;
        }
        else if (currentPlane == 7 && !mig31Sprite.isNull()) {
            currentSprite = &mig31Sprite;
        }

        QPointF center(currentSprite->width() / 2.0, currentSprite->height() / 2.0);
        painter.drawPixmap(-center, *currentSprite);
        painter.restore();

        // форсаж
        if (forsageAlpha > 0.01f && !forsageSprite.isNull()) {
            painter.save();
            painter.translate(posX, posY);
            painter.rotate(angle);

            painter.setOpacity(forsageAlpha);

            QPointF center(forsageSprite.width() / 2.0, forsageSprite.height() / 2.0);
            PlaneSkin &skin = skins[currentPlane];

            painter.save();
            painter.translate(skin.flameOffsetX, skin.flameOffsetY1);
            painter.rotate(-90);
            painter.drawPixmap(-center, forsageSprite);
            painter.restore();

            painter.save();
            painter.translate(skin.flameOffsetX, skin.flameOffsetY2);
            painter.rotate(-90);
            painter.drawPixmap(-center, forsageSprite);
            painter.restore();

            painter.setOpacity(1.0f);
            painter.restore();
        }

        painter.resetTransform();
        drawRadar(painter);

        // меню после посадки
        if (landingMenuOpen) {
            painter.save();
            painter.resetTransform();

            painter.fillRect(rect(), QColor(0, 0, 0, 200));

            painter.setPen(Qt::white);
            painter.setFont(QFont("Arial", 24, QFont::Bold));
            painter.drawText(rect().adjusted(0, -100, 0, 0), Qt::AlignHCenter, "ВЫБЕРИТЕ ДЕЙСТВИЕ | CHOOSE ACTION");

            painter.setFont(QFont("Arial", 18));

            if (landingChoice == 0) {
                painter.setPen(Qt::green);
                painter.drawText(rect().adjusted(0, 0, 0, 0), Qt::AlignHCenter, "> СВОБОДНЫЙ ПОЛЁT | FREE FLIGHT");
            } else {
                painter.setPen(Qt::white);
                painter.drawText(rect().adjusted(0, 0, 0, 0), Qt::AlignHCenter, "   СВОБОДНЫЙ ПОЛЁT | FREE FLIGHT");
            }

            if (landingChoice == 1) {
                painter.setPen(Qt::green);
                painter.drawText(rect().adjusted(0, 50, 0, 0), Qt::AlignHCenter, "> НОВАЯ МИССИЯ | NEW MISSION");
            } else {
                painter.setPen(Qt::white);
                painter.drawText(rect().adjusted(0, 50, 0, 0), Qt::AlignHCenter, "   НОВАЯ МИССИЯ | NEW MISSION");
            }

            painter.setPen(Qt::gray);
            painter.setFont(QFont("Arial", 14));
            painter.drawText(rect().adjusted(0, 150, 0, 0), Qt::AlignHCenter, "↑/↓ - выбор, ENTER - подтвердить | select, ENTER - confirm");

            painter.restore();
        }

        // отказ гидросистем
        if (hydraulicFailure && hydraulicTimer > 0) {
            painter.save();
            painter.resetTransform();

            painter.setPen(Qt::red);
            painter.setFont(QFont("Arial", 24, QFont::Bold));
            painter.drawText(rect().adjusted(0, -200, 0, 0), Qt::AlignHCenter, "! ОТКАЗ ГИДРОСИСТЕМ ! | HYDRAULIC FAILURE !");

            painter.setFont(QFont("Arial", 14));
            painter.setPen(Qt::white);
            painter.drawText(rect().adjusted(0, -150, 0, 0), Qt::AlignHCenter, "управление критически затруднено | control critically impaired");

            hydraulicTimer--;

            painter.restore();
        }

        if (shopOpen) {
            drawShop(painter);
        } else {
            int thrustPercent = (int)((thrust / 5.0f) * 100);
            if (thrustPercent > 100) thrustPercent = 100;

            painter.setPen(Qt::white);
            painter.setFont(QFont("Arial", 12));
            painter.drawText(10, 30, "топливо | fuel: " + QString::number(fuel, 'f', 1) + "%");
            painter.drawText(10, 50, "бомбы | bombs: " + QString::number(bombs));
            painter.drawText(10, 70, "деньги | money: " + QString::number(money));
            painter.drawText(10, 90, "броня | armor: " + QString::number(armor));
            painter.drawText(10, 110, "тяга | thrust: " + QString::number(thrustPercent) + "%");

            if (onGround) {
                painter.setPen(Qt::green);
                painter.drawText(10, 130, "на земле - заправка | on ground - refuel");
                painter.drawText(10, 150, "нажми M для магазина | press M for shop");
            } else {
                painter.setPen(Qt::white);
                painter.drawText(10, 130, "SPACE - бомба, R - ракета | SPACE - bomb, R - rocket");
            }

            if (fuel < 30 && !onGround) {
                painter.setPen(blink ? Qt::red : Qt::white);
                painter.setFont(QFont("Arial", 14, QFont::Bold));
                painter.drawText(width()/2 - 100, 100, "мало топлива | low fuel");
            }

            if (!onGround) {
                drawAirfieldArrow(painter);
            }
        }
    }

    void drawAirfieldArrow(QPainter &painter) {
        float dx = airfield.x - posX;
        float dy = airfield.y - posY;
        float angleToAirfield = atan2(dy, dx) * 180 / M_PI;
        float dist = sqrt(dx*dx + dy*dy);

        painter.save();
        painter.translate(100, height() - 100);

        painter.setPen(QPen(Qt::white, 2));
        painter.setBrush(QColor(0, 0, 0, 150));
        painter.drawEllipse(-30, -30, 60, 60);

        painter.save();
        painter.rotate(angleToAirfield - angle);
        painter.setBrush(Qt::green);
        painter.setPen(Qt::NoPen);
        painter.drawPolygon(QPolygon()
            << QPoint(0, -20)
            << QPoint(-8, 5)
            << QPoint(8, 5));
        painter.restore();

        painter.setPen(Qt::white);
        painter.setFont(QFont("Arial", 10));
        painter.drawText(-20, 40, QString::number((int)dist) + " м | m");

        painter.setPen(Qt::green);
        painter.setFont(QFont("Arial", 12, QFont::Bold));
        painter.drawText(-5, -35, "A");

        painter.restore();
    }

    void drawRadar(QPainter &painter) {
        painter.save();
        painter.translate(width() - 180, 80);

        painter.setBrush(QColor(0, 30, 0, 200));
        painter.setPen(QPen(Qt::green, 2));
        painter.drawEllipse(-60, -60, 120, 120);

        painter.setPen(QPen(Qt::green, 1, Qt::DotLine));
        painter.drawLine(-60, 0, 60, 0);
        painter.drawLine(0, -60, 0, 60);

        static float scanAngle = 0;
        scanAngle += 0.05f;

        painter.setPen(QPen(Qt::green, 2));
        painter.drawLine(0, 0, cos(scanAngle) * 55, sin(scanAngle) * 55);

        painter.setBrush(Qt::red);
        painter.setPen(Qt::NoPen);

        for (const Target &t : targets) {
            if (!t.alive) continue;

            float dx = t.x - posX;
            float dy = t.y - posY;
            float dist = sqrt(dx*dx + dy*dy);

            if (dist < 70000) {
                float radarX = (dx / 70000) * 50;
                float radarY = (dy / 70000) * 50;

                if (t.type == 2) {
                    painter.setBrush(Qt::red);
                } else {
                    painter.setBrush(Qt::yellow);
                }
                painter.drawEllipse(radarX - 3, radarY - 3, 6, 6);
            }
        }

        for (const EnemyPlane &e : enemies) {
            if (!e.alive) continue;
            float dx = e.x - posX;
            float dy = e.y - posY;
            float dist = sqrt(dx*dx + dy*dy);
            if (dist < 20000) {
                float radarX = (dx / 20000) * 50;
                float radarY = (dy / 20000) * 50;
                painter.setBrush(Qt::magenta);
                painter.drawEllipse(radarX - 3, radarY - 3, 6, 6);
            }
        }

        painter.setBrush(Qt::cyan);
        painter.drawEllipse(-4, -4, 8, 8);
        painter.restore();
    }

    void drawShop(QPainter &painter) {
        painter.fillRect(rect(), QColor(0, 0, 0, 220));

        painter.setPen(Qt::white);
        painter.setFont(QFont("Arial", 18, QFont::Bold));
        painter.drawText(rect().adjusted(0, 20, 0, 0), Qt::AlignHCenter, "Ангар | Hangar");

        painter.setFont(QFont("Arial", 12));

        for (int i = 0; i < skins.size(); i++) {
            int y = 130 + i * 90;

            if (i == selectedSkin) {
                painter.setPen(QPen(Qt::green, 3));
                painter.setBrush(QColor(0, 70, 0, 70));
            } else {
                painter.setPen(QPen(Qt::gray, 1));
                painter.setBrush(Qt::NoBrush);
            }
            painter.drawRect(150, y - 35, 700, 80);

            QPixmap mini;
            if (skins[i].owned || i == 0) {
                mini.load(skins[i].spriteFile);
                if (!mini.isNull()) {
                    mini = mini.scaled(60, 50, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                    painter.drawPixmap(160, y - 25, mini);
                }
            } else {
                painter.setBrush(Qt::darkGray);
                painter.drawRect(160, y - 25, 60, 50);
                painter.setPen(Qt::white);
                painter.drawText(180, y + 10, "?");
            }

            painter.setPen(Qt::white);
            painter.setFont(QFont("Arial", 14, QFont::Bold));
            painter.drawText(240, y - 10, skins[i].name);

            painter.setFont(QFont("Arial", 11));
            QString stats = "+" + QString::number(skins[i].speedBonus) + " скор | spd   " +
                QString::number(skins[i].armorBonus) + "+ броня | armor   " +
                QString::number(skins[i].bombBonus) + "+ бомб | bombs   " +
                QString::number(skins[i].turnSpeed) + " вращ | turn  ";
            painter.drawText(240, y + 30, stats);

            if (skins[i].owned) {
                if (i == currentPlane) {
                    painter.setPen(Qt::green);
                    painter.drawText(680, y, "выбран | selected");
                } else {
                    painter.setPen(Qt::green);
                    painter.drawText(680, y, "куплен | owned");
                }
            } else {
                painter.setPen(Qt::yellow);
                painter.drawText(550, y, QString::number(skins[i].price) + " $");
            }
        }

        painter.setPen(Qt::white);
        painter.setFont(QFont("Arial", 12));
        painter.drawText(rect().adjusted(0, 50, 0, 0), Qt::AlignHCenter, "M - выход, ↑/↓ - выбор, ENTER - купить/выбрать | M - exit, ↑/↓ - select, ENTER - buy/select");
    }

    void keyPressEvent(QKeyEvent *event) override {
        if (event->key() == Qt::Key_Escape) {
            qApp->quit();
        }

        if (shopOpen) {
            if (event->key() == Qt::Key_M) {
                shopOpen = false;
                return;
            }

            if (event->key() == Qt::Key_Up) {
                selectedSkin--;
                if (selectedSkin < 0) selectedSkin = skins.size() - 1;
                update();
                return;
            }

            if (event->key() == Qt::Key_Down) {
                selectedSkin++;
                if (selectedSkin >= skins.size()) selectedSkin = 0;
                update();
                return;
            }

            if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
                if (!skins[selectedSkin].owned) {
                    if (money >= skins[selectedSkin].price) {
                        money -= skins[selectedSkin].price;
                        skins[selectedSkin].owned = true;
                        currentPlane = selectedSkin;
                        saveMoney();
                        saveHangar();
                    }
                } else {
                    currentPlane = selectedSkin;
                    saveHangar();
                }
                update();
                return;
            }
            return;
        }

        if (event->key() == Qt::Key_W) keyW = true;
        if (event->key() == Qt::Key_S) keyS = true;
        if (event->key() == Qt::Key_A) keyA = true;
        if (event->key() == Qt::Key_D) keyD = true;

        if (event->key() == Qt::Key_M && onGround) {
            shopOpen = true;
            selectedSkin = currentPlane;
        }

        // 🚀 ЗАПУСК РАКЕТЫ
        if (event->key() == Qt::Key_R && !onGround && money >= 10) {
            money -= 10;
            saveMoney();

            float rad = angle * M_PI / 180.0f;

            EnemyPlane *nearestEnemy = nullptr;
            Target *nearestTarget = nullptr;
            float minDist = 999999;

            for (EnemyPlane &e : enemies) {
                if (!e.alive) continue;
                float dist = sqrt(pow(posX - e.x, 2) + pow(posY - e.y, 2));
                if (dist < minDist) {
                    minDist = dist;
                    nearestEnemy = &e;
                }
            }

            if (!nearestEnemy) {
                for (Target &t : targets) {
                    if (!t.alive) continue;
                    float dist = sqrt(pow(posX - t.x, 2) + pow(posY - t.y, 2));
                    if (dist < minDist) {
                        minDist = dist;
                        nearestTarget = &t;
                    }
                }
            }

            Rocket r;
            r.x = posX + cos(rad) * 50;
            r.y = posY + sin(rad) * 50;
            r.vx = cos(rad) * 40.0f;
            r.vy = sin(rad) * 40.0f;
            r.timer = 300;
            r.target = nearestTarget;
            r.enemyTarget = nearestEnemy;
            r.isHoming = true;

            rockets.append(r);
        }

        if (landingMenuOpen) {
            if (event->key() == Qt::Key_Up) {
                landingChoice = 0;
                update();
                return;
            }
            if (event->key() == Qt::Key_Down) {
                landingChoice = 1;
                update();
                return;
            }
            if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
                if (landingChoice == 0) {
                    fuel = 100;
                    bombs = 50 + skins[currentPlane].bombBonus;
                    armor = 100 + skins[currentPlane].armorBonus;
                    money += 50;
                    saveMoney();
                    landingMenuOpen = false;
                } else {
                    fuel = 100;
                    bombs = 50 + skins[currentPlane].bombBonus;
                    armor = 100 + skins[currentPlane].armorBonus;
                    money += 50;
                    saveMoney();

                    targets.clear();
                    enemies.clear();

                    for (int i = 0; i < 10; i++) {
                        float x = 1000 + rand() % 126000;
                        float y = 1000 + rand() % 126000;
                        targets.append(Target(x, y, (rand() % 2 == 0) ? 0 : 2));
                    }
                    for (int i = 0; i < 10; i++) {
                        float x = 1000 + rand() % 126000;
                        float y = 1000 + rand() % 126000;
                        enemies.append(EnemyPlane(x, y, rand() % 2));
                    }

                    landingMenuOpen = false;
                }
                update();
                return;
            }
            return;
        }
    }

    void keyReleaseEvent(QKeyEvent *event) override {
        if (shopOpen) return;

        if (event->key() == Qt::Key_W) keyW = false;
        if (event->key() == Qt::Key_S) keyS = false;
        if (event->key() == Qt::Key_A) keyA = false;
        if (event->key() == Qt::Key_D) keyD = false;

        if (event->key() == Qt::Key_Space && bombs > 0 && !onGround) {
            dropBomb();
        }
    }

private slots:
    void updateGame() {
        if (shopOpen) return;

        // плавное затухание форсажа
        if (thrust > 3.0f && !onGround) {
            forsageAlpha += 0.02f;
            if (forsageAlpha > 1.0f) forsageAlpha = 1.0f;
        } else {
            forsageAlpha -= 0.02f;
            if (forsageAlpha < 0.0f) forsageAlpha = 0.0f;
        }

        // отказ гидросистем
        if (armor <= 30 && armor > 0 && !hydraulicFailure) {
            hydraulicFailure = true;
            hydraulicTimer = 120;
        }

        int speedBonus = hydraulicFailure ? 0 : skins[currentPlane].speedBonus;
        int armorBonus = skins[currentPlane].armorBonus;
        float turnSpeed = hydraulicFailure ? 1.0f : skins[currentPlane].turnSpeed;

        if (keyW) thrust = std::min(thrust + 0.1f, 5.0f + speedBonus);
        if (keyS) thrust = std::max(thrust - 0.1f, 0.0f);

        if (keyA) angle -= turnSpeed;
        if (keyD) angle += turnSpeed;
        if (angle >= 360) angle -= 360;
        if (angle < 0) angle += 360;

        if (thrust > 0 && fuel > 0 && !onGround) {
            fuel -= 0.01f;
            if (fuel < 0) fuel = 0;
        }

        if (fuel < 20 && fuel > 0 && !onGround && !fuelWarningPlayed) {
            QApplication::beep();
            fuelWarningPlayed = true;
        }
        if (fuel >= 20 || onGround) {
            fuelWarningPlayed = false;
        }

        float rad = angle * M_PI / 180.0f;
        QVector2D direction(cos(rad), sin(rad));

        if (fuel > 0) {
            velocity += direction * thrust * 0.05f;
        }

        velocity *= onGround ? 0.95f : 0.99f;

        posX += velocity.x();
        posY += velocity.y();

        if (posX < 0) posX = 0;
        if (posX > 128000) posX = 128000;
        if (posY < 0) posY = 0;
        if (posY > 128000) posY = 128000;

        // логика зениток
        for (Target &t : targets) {
            if (!t.alive || t.type != 2) continue;

            float dx = posX - t.x;
            float dy = posY - t.y;
            t.angle = atan2(dy, dx);

            float dist = sqrt(dx*dx + dy*dy);
            if (dist < 600 && rand() % 30 == 0) {
                AABullet b;
                b.x = t.x;
                b.y = t.y;
                float speed = 100.0f;
                b.vx = cos(t.angle) * speed;
                b.vy = sin(t.angle) * speed;
                b.timer = 100;
                AABullets.append(b);
            }
        }

        // движение пуль зениток
        for (int i = AABullets.size() - 1; i >= 0; i--) {
            AABullet &b = AABullets[i];
            b.x += b.vx;
            b.y += b.vy;
            b.timer--;

            float dist = sqrt(pow(posX - b.x, 2) + pow(posY - b.y, 2));
            if (dist < 15 && !onGround) {
                armor -= 10;
                hitMarkers.append({posX, posY, 10});
                AABullets.removeAt(i);
                if (armor <= 0) {
                    posX = 4000;
                    posY = 4000;
                    armor = 100 + armorBonus;
                    money = 0;
                    bombs = 50;
                    fuel = 100;
                    hydraulicFailure = false;
                    saveMoney();
                }
                continue;
            }

            if (b.timer <= 0 || b.x < 0 || b.x > 8000 || b.y < 0 || b.y > 8000) {
                AABullets.removeAt(i);
            }
        }

        // зенитки по врагам
        for (int i = AABullets.size() - 1; i >= 0; i--) {
            AABullet &b = AABullets[i];

            for (EnemyPlane &e : enemies) {
                if (!e.alive) continue;
                float distToEnemy = sqrt(pow(b.x - e.x, 2) + pow(b.y - e.y, 2));
                if (distToEnemy < 15) {
                    e.hp -= 1;
                    hitMarkers.append({e.x, e.y, 10});
                    if (e.hp <= 0) {
                        e.alive = false;
                    }
                    AABullets.removeAt(i);
                    break;
                }
            }
        }

        // логика врагов
        for (EnemyPlane &e : enemies) {
            if (!e.alive) continue;

            float dx = posX - e.x;
            float dy = posY - e.y;
            float dist = sqrt(dx*dx + dy*dy);

            if (dist < 1200) {
                float targetAngle = atan2(dy, dx) * 180 / M_PI;
                float angleDiff = targetAngle - e.angle;
                while (angleDiff > 180) angleDiff -= 360;
                while (angleDiff < -180) angleDiff += 360;

                float turnSpeed = 5.0f;
                if (angleDiff > turnSpeed) angleDiff = turnSpeed;
                if (angleDiff < -turnSpeed) angleDiff = -turnSpeed;
                e.angle += angleDiff;

                float targetSpeed = 8.5f;
                float rad = e.angle * M_PI / 180.0f;
                QVector2D direction(cos(rad), sin(rad));

                e.vx += direction.x() * 0.1f;
                e.vy += direction.y() * 0.1f;

                float currentSpeed = sqrt(e.vx*e.vx + e.vy*e.vy);
                if (currentSpeed > targetSpeed) {
                    e.vx = (e.vx / currentSpeed) * targetSpeed;
                    e.vy = (e.vy / currentSpeed) * targetSpeed;
                }

                e.shootTimer++;
                if (e.shootTimer > 45 && dist < 700) {
                    e.shootTimer = 0;

                    Rocket r;
                    float rad = e.angle * M_PI / 180.0f;
                    r.x = e.x + cos(rad) * 30;
                    r.y = e.y + sin(rad) * 30;
                    r.vx = cos(rad) * 40.0f;
                    r.vy = sin(rad) * 40.0f;
                    r.timer = 200;
                    r.target = nullptr;
                    r.enemyTarget = nullptr;  // враги стреляют по игроку
                    r.isHoming = true;
                    enemyRockets.append(r);
                }
            } else {
                e.angle += 0.5f;
                float rad = e.angle * M_PI / 180.0f;
                QVector2D direction(cos(rad), sin(rad));

                e.vx += direction.x() * 0.05f;
                e.vy += direction.y() * 0.05f;

                float currentSpeed = sqrt(e.vx*e.vx + e.vy*e.vy);
                if (currentSpeed > 3.0f) {
                    e.vx = (e.vx / currentSpeed) * 3.0f;
                    e.vy = (e.vy / currentSpeed) * 3.0f;
                }
            }

            e.x += e.vx;
            e.y += e.vy;
            e.vx *= 0.99f;
            e.vy *= 0.99f;

            if (e.x < 0) { e.x = 128000; e.vx = 0; }
            if (e.x > 128000) { e.x = 0; e.vx = 0; }
            if (e.y < 0) { e.y = 128000; e.vy = 0; }
            if (e.y > 128000) { e.y = 0; e.vy = 0; }
        }

        // 🚀 ДВИЖЕНИЕ РАКЕТ ИГРОКА
        for (int i = rockets.size() - 1; i >= 0; i--) {
            Rocket &r = rockets[i];

            // наведение на цель
            if (r.isHoming) {
                float targetX = 0, targetY = 0;
                bool hasTarget = false;

                if (r.enemyTarget && r.enemyTarget->alive) {
                    targetX = r.enemyTarget->x;
                    targetY = r.enemyTarget->y;
                    hasTarget = true;
                } else if (r.target && r.target->alive) {
                    targetX = r.target->x;
                    targetY = r.target->y;
                    hasTarget = true;
                }

                if (hasTarget) {
                    float dx = targetX - r.x;
                    float dy = targetY - r.y;
                    float distToTarget = sqrt(dx*dx + dy*dy);

                    if (distToTarget > 20) {
                        float targetAngle = atan2(dy, dx);
                        float currentAngle = atan2(r.vy, r.vx);

                        float angleDiff = targetAngle - currentAngle;
                        while (angleDiff > M_PI) angleDiff -= 2*M_PI;
                        while (angleDiff < -M_PI) angleDiff += 2*M_PI;

                        currentAngle += angleDiff * 0.05f;

                        float speed = sqrt(r.vx*r.vx + r.vy*r.vy);
                        r.vx = cos(currentAngle) * speed;
                        r.vy = sin(currentAngle) * speed;
                    }
                } else {
                    r.isHoming = false;
                }
            }

            r.x += r.vx;
            r.y += r.vy;
            r.timer--;

            bool hit = false;

            // по врагам
            for (EnemyPlane &e : enemies) {
                if (!e.alive) continue;
                float dist = sqrt(pow(r.x - e.x, 2) + pow(r.y - e.y, 2));
                if (dist < 20) {
                    e.hp -= 3;
                    hitMarkers.append({e.x, e.y, 15});
                    if (e.hp <= 0) {
                        e.alive = false;
                        money += 50;
                        saveMoney();
                    }
                    rockets.removeAt(i);
                    hit = true;
                    break;
                }
            }

            if (hit) continue;

            // по наземным целям
            for (Target &t : targets) {
                if (!t.alive) continue;
                float dist = sqrt(pow(r.x - t.x, 2) + pow(r.y - t.y, 2));
                if (dist < 20) {
                    t.hp -= 5;
                    hitMarkers.append({t.x, t.y, 15});
                    if (t.hp <= 0) {
                        t.alive = false;
                        money += 10;
                        saveMoney();
                    }
                    rockets.removeAt(i);
                    hit = true;
                    break;
                }
            }

            if (hit) continue;

            if (r.timer <= 0 || r.x < 0 || r.x > 128000 || r.y < 0 || r.y > 128000) {
                rockets.removeAt(i);
            }
        }

        // 🚀 ДВИЖЕНИЕ РАКЕТ ВРАГОВ
        for (int i = enemyRockets.size() - 1; i >= 0; i--) {
            Rocket &r = enemyRockets[i];

            // наведение на игрока
            if (r.isHoming) {
                float dx = posX - r.x;
                float dy = posY - r.y;
                float distToPlayer = sqrt(dx*dx + dy*dy);

                if (distToPlayer > 20) {
                    float targetAngle = atan2(dy, dx);
                    float currentAngle = atan2(r.vy, r.vx);

                    float angleDiff = targetAngle - currentAngle;
                    while (angleDiff > M_PI) angleDiff -= 2*M_PI;
                    while (angleDiff < -M_PI) angleDiff += 2*M_PI;

                    currentAngle += angleDiff * 0.05f;

                    float speed = sqrt(r.vx*r.vx + r.vy*r.vy);
                    r.vx = cos(currentAngle) * speed;
                    r.vy = sin(currentAngle) * speed;
                }
            }

            r.x += r.vx;
            r.y += r.vy;
            r.timer--;

            float distToPlayer = sqrt(pow(posX - r.x, 2) + pow(posY - r.y, 2));
            if (distToPlayer < 20 && !onGround) {
                armor -= 20;
                hitMarkers.append({posX, posY, 15});
                enemyRockets.removeAt(i);
                if (armor <= 0) {
                    posX = 4000;
                    posY = 4000;
                    armor = 100 + armorBonus;
                    money = 0;
                    bombs = 50;
                    fuel = 100;
                    hydraulicFailure = false;
                    saveMoney();
                }
                continue;
            }

            if (r.timer <= 0 || r.x < 0 || r.x > 128000 || r.y < 0 || r.y > 128000) {
                enemyRockets.removeAt(i);
            }
        }

        for (int i = hitMarkers.size() - 1; i >= 0; i--) {
            hitMarkers[i].timer--;
            if (hitMarkers[i].timer <= 0) hitMarkers.removeAt(i);
        }

        float distToAirfield = sqrt(pow(posX - airfield.x, 2) + pow(posY - airfield.y, 2));
        bool wasOnGround = onGround;
        onGround = (distToAirfield < 600 && velocity.length() < 3.0f);

        if (onGround && !wasOnGround) {
            fuel = 100;
            bombs = 50 + skins[currentPlane].bombBonus;
            armor = 100 + skins[currentPlane].armorBonus;
            money += 50;

            hydraulicFailure = false;
            hydraulicTimer = 0;

            saveMoney();
            landingMenuOpen = true;
            landingChoice = 0;
        }
    }

    void dropBomb() {
        bombs--;
        for (Target &t : targets) {
            if (!t.alive) continue;
            float dist = sqrt(pow(posX - t.x, 2) + pow(posY - t.y, 2));
            if (dist < 60) {
                t.hp--;
                hitMarkers.append({t.x, t.y, 10});

                if (t.hp <= 0) {
                    t.alive = false;
                    money += 20;

                    if (t.type == 2) {
                        money += 30;
                    }
                    saveMoney();
                }
            }
        }
    }

    void loadMoney() {
        QFile file("money.txt");
        if (file.open(QIODevice::ReadOnly)) {
            QTextStream in(&file);
            in >> money;
            file.close();
        }
    }

    void saveMoney() {
        QFile file("money.txt");
        if (file.open(QIODevice::WriteOnly)) {
            QTextStream out(&file);
            out << money;
            file.close();
        }
    }

private:
    float posX, posY;
    QVector2D velocity;
    float angle, thrust, fuel;
    int bombs, money, armor;
    bool keyW, keyS, keyA, keyD, onGround;
    bool blink, fuelWarningPlayed;
    bool shopOpen;
    int selectedSkin, currentPlane;
    QList<QPointF> trail;

    bool landingMenuOpen;
    int landingChoice;
    bool hydraulicFailure;
    int hydraulicTimer;
    float forsageAlpha;

    QPixmap su27Sprite, su30Sprite, su33Sprite, su25Sprite, su34Sprite, su57Sprite, mig31Sprite, mig29Sprite;
    QPixmap tankSprite, rocketSprite, AASprite, treeSprite, grassTexture, asphaltTexture;
    QPixmap eurofighterSprite, f18Sprite;
    QPixmap forsageSprite;

    QVector<Target> targets;
    QVector<Rocket> rockets;
    QVector<Rocket> enemyRockets;
    QVector<HitMarker> hitMarkers;
    QVector<AABullet> AABullets;
    QVector<PlaneSkin> skins;
    QVector<GrassBlade> grass;
    QVector<QPointF> trees;
    QVector<EnemyPlane> enemies;

    Airfield airfield;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    AirplaneGame game;
    game.show();
    return app.exec();
}
