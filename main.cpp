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

// === СТРУКТУРЫ ДАННЫХ ===
struct Target {
    float x, y;
    bool alive;
    int type;  // 0 - танк, 2 - пво
    int hp;
    float angle;

    Target(float x_, float y_, int t) : x(x_), y(y_), alive(true), type(t), hp(3), angle(0) {}
};

struct HitMarker {
    float x, y;
    int timer;
};

struct PvoBullet {
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

struct Rocket {
    float x, y;
    float vx, vy;
    int timer;
    Target *target;
};

struct PlaneSkin {
    QString name;
    int price;
    int speedBonus;
    int armorBonus;
    int bombBonus;
    float turnSpeed;
    bool owned;
    QString spriteFile;
};

// === ОСНОВНОЙ КЛАСС ИГРЫ ===
class AirplaneGame : public QWidget
{
public:
    AirplaneGame(QWidget *parent = nullptr)
        : QWidget(parent),
          posX(7850), posY(8010),
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
          airfield(8000, 8000),
          shopOpen(false),
          selectedSkin(0),
          currentPlane(0)
    {
        setFixedSize(1920, 900);
        setWindowTitle("авиасимулятор - генерация");

        // === ЗАГРУЗКА СПРАЙТОВ ===
        su27Sprite.load("plane.png");
        if (!su27Sprite.isNull()) {
            su27Sprite = su27Sprite.scaled(80, 80, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }

        su30Sprite.load("plane1.png");
        if (!su30Sprite.isNull()) {
            su30Sprite = su30Sprite.scaled(85, 85, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }

        su25Sprite.load("plane4.png");
        if (!su25Sprite.isNull()) {
            su25Sprite = su25Sprite.scaled(85, 85, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }

        su34Sprite.load("plane2.png");
        if (!su34Sprite.isNull()) {
            su34Sprite = su34Sprite.scaled(120, 120, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }

        su57Sprite.load("plane3.png");
        if (!su57Sprite.isNull()) {
            su57Sprite = su57Sprite.scaled(90, 90, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }

        grassTexture.load("grass_texture.png");
        if (!grassTexture.isNull()) {
            grassTexture = grassTexture.scaled(128, 128, Qt::KeepAspectRatio);
        }

        asphaltTexture.load("asphalt.png");
        if (!asphaltTexture.isNull()) {
            asphaltTexture = asphaltTexture.scaled(128, 128, Qt::KeepAspectRatio);
        }

        tankSprite.load("tank.png");
        if (!tankSprite.isNull()) {
            tankSprite = tankSprite.scaled(50, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }

        pvoSprite.load("pvo.png");
        if (!pvoSprite.isNull()) {
            pvoSprite = pvoSprite.scaled(50, 50, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }

        treeSprite.load("tree.png");
        if (!treeSprite.isNull()) {
            treeSprite = treeSprite.scaled(80, 80, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }

        rocketSprite.load("rocket.png");
        if (!rocketSprite.isNull()) {
            rocketSprite = rocketSprite.scaled(40, 30, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }

        // === НАСТРОЙКА МАГАЗИНА ===
        skins.append({"СУ-27", 0, 2, 0, 0, 3.0f, true, "plane.png"});
        skins.append({"СУ-30", 2000, 3, 10, 0, 3.0f, false, "plane1.png"});
        skins.append({"СУ-34", 3000, 3, 15, 40, 2.5f, false, "plane2.png"});
        skins.append({"СУ-25", 3500, 2, 25, 50, 2.0f, false, "plane4.png"});
        skins.append({"СУ-57", 5000, 5, 20, 60, 3.5f, false, "plane3.png"});

        loadMoney();

        // === ГЕНЕРАЦИЯ МИРА ===
        for (int i = 0; i < 20; i++) {
            float x = 2000 + rand() % 12000;
            float y = 2000 + rand() % 12000;
            targets.append(Target(x, y, (rand() % 2 == 0) ? 0 : 2));
        }

        for (int i = 0; i < 1000; i++) {
            float x = 1000 + rand() % 14000;
            float y = 1000 + rand() % 14000;
            trees.append(QPointF(x, y));
        }

        // === ТАЙМЕРЫ ===
        QTimer *timer = new QTimer(this);
        connect(timer, &QTimer::timeout, [this]() { updateGame(); });
        connect(timer, &QTimer::timeout, this, QOverload<>::of(&AirplaneGame::update));
        timer->start(20);

        QTimer *blinkTimer = new QTimer(this);
        connect(blinkTimer, &QTimer::timeout, [this]() { blink = !blink; });
        blinkTimer->start(500);

        setFocusPolicy(Qt::StrongFocus);
    }

protected:
    // === ОТРИСОВКА ===
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // камера
        float camX = posX - width()/2;
        float camY = posY - height()/2;
        if (camX < 0) camX = 0;
        if (camY < 0) camY = 0;
        if (camX > 16000 - width()) camX = 16000 - width();
        if (camY > 16000 - height()) camY = 16000 - height();
        painter.translate(-camX, -camY);

        // фон
        if (!grassTexture.isNull()) {
            painter.setBrush(QBrush(grassTexture));
            painter.setPen(Qt::NoPen);
            painter.drawRect(0, 0, 16000, 16000);
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

        // ракеты
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

        // огни (поверх асфальта)
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
                if (!pvoSprite.isNull()) {
                    QPointF center(pvoSprite.width() / 2.0, pvoSprite.height() / 2.0);
                    painter.drawPixmap(-center, pvoSprite);
                    painter.setPen(QPen(Qt::black, 3));
                    painter.drawLine(0, 0, cos(t.angle) * 25, sin(t.angle) * 25);
                }
            }
            painter.restore();

            // полоска здоровья
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

        // пули пво
        painter.setBrush(Qt::yellow);
        painter.setPen(Qt::NoPen);
        for (const PvoBullet &b : pvoBullets) {
            painter.drawEllipse(b.x - 2, b.y - 2, 4, 4);
        }

        // попадания
        for (const HitMarker &h : hitMarkers) {
            float progress = h.timer / 10.0;
            painter.setPen(QPen(QColor(255, 100, 0, 255 * progress), 2));
            painter.drawLine(h.x - 8, h.y - 8, h.x + 8, h.y + 8);
            painter.drawLine(h.x + 8, h.y - 8, h.x - 8, h.y + 8);
        }

        // === ОТРИСОВКА САМОЛЁТА ===
        painter.save();
        painter.translate(posX, posY);
        painter.rotate(angle + 90);

        QPixmap *currentSprite = &su27Sprite;

        if (currentPlane == 1 && !su30Sprite.isNull()) {
            currentSprite = &su30Sprite;
        }
        else if (currentPlane == 2 && !su34Sprite.isNull()) {
            currentSprite = &su34Sprite;
        }
        else if (currentPlane == 3 && !su25Sprite.isNull()) {
            currentSprite = &su25Sprite;
        }
        else if (currentPlane == 4 && !su57Sprite.isNull()) {
            currentSprite = &su57Sprite;
        }

        QPointF center(currentSprite->width() / 2.0, currentSprite->height() / 2.0);
        painter.drawPixmap(-center, *currentSprite);
        painter.restore();

        // след
        if (trail.size() > 1) {
            painter.setPen(QPen(Qt::white, 1));
            for (int i = 1; i < trail.size(); ++i) {
                painter.drawLine(trail[i-1], trail[i]);
            }
        }

        painter.resetTransform();

        // интерфейсы
        drawRadar(painter);

        if (shopOpen) {
            drawShop(painter);
        } else {
            painter.setPen(Qt::white);
            painter.setFont(QFont("Arial", 12));
            painter.drawText(10, 30, "топливо: " + QString::number(fuel, 'f', 1) + "%");
            painter.drawText(10, 50, "бомбы: " + QString::number(bombs));
            painter.drawText(10, 70, "деньги: " + QString::number(money));
            painter.drawText(10, 90, "броня: " + QString::number(armor));

            if (onGround) {
                painter.setPen(Qt::green);
                painter.drawText(10, 120, "на земле - заправка");
                painter.drawText(10, 140, "нажми M для магазина");
            } else {
                painter.setPen(Qt::white);
                painter.drawText(10, 120, "SPACE - бомба, R - ракета");
            }

            if (fuel < 30 && !onGround) {
                painter.setPen(blink ? Qt::red : Qt::white);
                painter.setFont(QFont("Arial", 14, QFont::Bold));
                painter.drawText(width()/2 - 100, 100, "мало топлива!");
            }

            if (!onGround) {
                drawAirfieldArrow(painter);
            }
        }
    }

    // === ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ОТРИСОВКИ ===
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
        painter.drawText(-20, 40, QString::number((int)dist) + " м");

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
            if (dist < 6000) {
                float radarX = (dx / 6000) * 50;
                float radarY = (dy / 6000) * 50;

                if (t.type == 2) {
                    painter.setBrush(Qt::red);
                } else {
                    painter.setBrush(Qt::yellow);
                }
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
        painter.drawText(rect().adjusted(0, 20, 0, 0), Qt::AlignHCenter, "Ангар");

        painter.setFont(QFont("Arial", 12));

        for (int i = 0; i < skins.size(); i++) {
            int y = 130 + i * 90;

            if (i == selectedSkin) {
                painter.setPen(QPen(Qt::green, 3));
                painter.setBrush(QColor(0, 50, 0, 50));
            } else {
                painter.setPen(QPen(Qt::gray, 1));
                painter.setBrush(Qt::NoBrush);
            }
            painter.drawRect(150, y - 35, 500, 80);

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
            QString stats = "+" + QString::number(skins[i].speedBonus) + " скор.   " +
                QString::number(skins[i].armorBonus) + "+ броня   " +
                QString::number(skins[i].bombBonus) + "+ бомб   " +
                QString::number(skins[i].turnSpeed) + " вращ.  ";
            painter.drawText(240, y + 10, stats);

            if (skins[i].owned) {
                if (i == currentPlane) {
                    painter.setPen(Qt::green);
                    painter.drawText(550, y, "выбран");
                } else {
                    painter.setPen(Qt::green);
                    painter.drawText(550, y, "куплен");
                }
            } else {
                painter.setPen(Qt::yellow);
                painter.drawText(550, y, QString::number(skins[i].price) + " $");
            }
        }

        painter.setPen(Qt::white);
        painter.setFont(QFont("Arial", 12));
        painter.drawText(300, 570, "M - выход, ↑/↓ - выбор, ENTER - купить/выбрать");
    }

    // === ОБРАБОТКА НАЖАТИЙ ===
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
                    }
                } else {
                    currentPlane = selectedSkin;
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

        // === ЗАПУСК РАКЕТЫ ===
        if (event->key() == Qt::Key_R && !onGround && money >= 10) {
            money -= 10;
            saveMoney();

            float rad = angle * M_PI / 180.0f;

            Target *nearest = nullptr;
            float minDist = 999999;

            for (Target &t : targets) {
                if (!t.alive) continue;
                float dist = sqrt(pow(posX - t.x, 2) + pow(posY - t.y, 2));
                if (dist < minDist) {
                    minDist = dist;
                    nearest = &t;
                }
            }

            if (nearest) {
                Rocket r;
                r.x = posX + cos(rad) * 50;
                r.y = posY + sin(rad) * 50;

                float dx = nearest->x - r.x;
                float dy = nearest->y - r.y;
                float dist = sqrt(dx*dx + dy*dy);

                float rocketSpeed = 50.0f;
                r.vx = (dx / dist) * rocketSpeed;
                r.vy = (dy / dist) * rocketSpeed;

                r.timer = 300;
                r.target = nearest;
                rockets.append(r);
            }
        }
    }

    void keyReleaseEvent(QKeyEvent *event) override {
        if (shopOpen) return;

        if (event->key() == Qt::Key_W) keyW = false;
        if (event->key() == Qt::Key_S) keyS = false;
        if (event->key() == Qt::Key_A) keyA = false;
        if (event->key() == Qt::Key_D) keyD = false;

        // === СБРОС БОМБЫ ===
        if (event->key() == Qt::Key_Space && bombs > 0 && !onGround) {
            dropBomb();
        }
    }

private slots:
    // === ОСНОВНАЯ ЛОГИКА ИГРЫ (ФИЗИКА, ДВИЖЕНИЕ, ПРОВЕРКИ) ===
    void updateGame() {
        if (shopOpen) return;

        int speedBonus = skins[currentPlane].speedBonus;
        int armorBonus = skins[currentPlane].armorBonus;

        // управление тягой
        if (keyW) thrust = std::min(thrust + 0.1f, 5.0f + speedBonus);
        if (keyS) thrust = std::max(thrust - 0.1f, 0.0f);

        // поворот
        float currentTurnSpeed = skins[currentPlane].turnSpeed;
        if (keyA) angle -= currentTurnSpeed;
        if (keyD) angle += currentTurnSpeed;
        if (angle >= 360) angle -= 360;
        if (angle < 0) angle += 360;

        // расход топлива
        if (thrust > 0 && fuel > 0 && !onGround) {
            fuel -= 0.01f;
            if (fuel < 0) fuel = 0;
        }

        // звуковое предупреждение о топливе
        if (fuel < 20 && fuel > 0 && !onGround && !fuelWarningPlayed) {
            QApplication::beep();
            fuelWarningPlayed = true;
        }
        if (fuel >= 20 || onGround) {
            fuelWarningPlayed = false;
        }

        // физика движения
        float rad = angle * M_PI / 180.0f;
        QVector2D direction(cos(rad), sin(rad));

        if (fuel > 0) {
            velocity += direction * thrust * 0.05f;
        }

        velocity *= onGround ? 0.95f : 0.99f;

        posX += velocity.x();
        posY += velocity.y();

        // границы карты
        if (posX < 0) posX = 0;
        if (posX > 16000) posX = 16000;
        if (posY < 0) posY = 0;
        if (posY > 16000) posY = 16000;

        // === ЛОГИКА ПВО ===
        for (Target &t : targets) {
            if (!t.alive || t.type != 2) continue;

            float dx = posX - t.x;
            float dy = posY - t.y;
            t.angle = atan2(dy, dx);

            float dist = sqrt(dx*dx + dy*dy);
            if (dist < 300 && rand() % 30 == 0) {
                PvoBullet b;
                b.x = t.x;
                b.y = t.y;
                float speed = 5.0f;
                b.vx = cos(t.angle) * speed;
                b.vy = sin(t.angle) * speed;
                b.timer = 50;
                pvoBullets.append(b);
            }
        }

        // движение пуль пво
        for (int i = pvoBullets.size() - 1; i >= 0; i--) {
            PvoBullet &b = pvoBullets[i];
            b.x += b.vx;
            b.y += b.vy;
            b.timer--;

            float dist = sqrt(pow(posX - b.x, 2) + pow(posY - b.y, 2));
            if (dist < 15 && !onGround) {
                armor -= 10;
                hitMarkers.append({posX, posY, 10});
                pvoBullets.removeAt(i);
                if (armor <= 0) {
                    posX = 4000;
                    posY = 4000;
                    armor = 100 + armorBonus;
                    money = 0;
                    bombs = 50;
                    fuel = 100;
                    saveMoney();
                }
                continue;
            }

            if (b.timer <= 0 || b.x < 0 || b.x > 8000 || b.y < 0 || b.y > 8000) {
                pvoBullets.removeAt(i);
            }
        }

        // анимация попаданий
        for (int i = hitMarkers.size() - 1; i >= 0; i--) {
            hitMarkers[i].timer--;
            if (hitMarkers[i].timer <= 0) hitMarkers.removeAt(i);
        }

        // === ПОСАДКА И ЗАПРАВКА ===
        float distToAirfield = sqrt(pow(posX - airfield.x, 2) + pow(posY - airfield.y, 2));
        bool wasOnGround = onGround;
        onGround = (distToAirfield < 600 && velocity.length() < 3.0f);

        if (onGround && !wasOnGround) {
            fuel = 100;
            bombs = 50 + skins[currentPlane].bombBonus;
            armor = 100 + skins[currentPlane].armorBonus;
            money += 50;
            saveMoney();

            for (int i = 0; i < 20; i++) {
                float x = 1000 + rand() % 6000;
                float y = 1000 + rand() % 6000;
                targets.append(Target(x, y, (rand() % 2 == 0) ? 0 : 2));
            }
        }

        if (wasOnGround && velocity.length() > 3.0f) onGround = false;

        // след
        if (!onGround) {
            trail.push_back(QPointF(posX, posY));
            if (trail.size() > 300) trail.removeFirst();
        }

        // === ДВИЖЕНИЕ РАКЕТ ===
        for (int i = rockets.size() - 1; i >= 0; i--) {
            Rocket &r = rockets[i];

            if (r.target && r.target->alive) {
                float dx = r.target->x - r.x;
                float dy = r.target->y - r.y;
                float dist = sqrt(dx*dx + dy*dy);

                if (dist < 20) {
                    r.target->hp -= 5;
                    hitMarkers.append({r.target->x, r.target->y, 15});

                    if (r.target->hp <= 0) {
                        r.target->alive = false;
                        money += 10;
                        saveMoney();
                    }
                    rockets.removeAt(i);
                    continue;
                }

                float speed = 50.0f;
                r.vx = (dx / dist) * speed;
                r.vy = (dy / dist) * speed;
            }

            r.x += r.vx;
            r.y += r.vy;
            r.timer--;

            if (r.timer <= 0 || r.x < 0 || r.x > 16000 || r.y < 0 || r.y > 16000) {
                rockets.removeAt(i);
            }
        }
    }

    // === СБРОС БОМБЫ ===
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

    // === СОХРАНЕНИЕ И ЗАГРУЗКА ДЕНЕГ ===
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
    // === ПЕРЕМЕННЫЕ ИГРОКА ===
    float posX, posY;
    QVector2D velocity;
    float angle, thrust, fuel;
    int bombs, money, armor;
    bool keyW, keyS, keyA, keyD, onGround;
    bool blink, fuelWarningPlayed;
    bool shopOpen;
    int selectedSkin, currentPlane;
    QList<QPointF> trail;

    // === СПРАЙТЫ ===
    QPixmap su27Sprite;
    QPixmap su30Sprite;
    QPixmap su25Sprite;
    QPixmap su34Sprite;
    QPixmap su57Sprite;
    QPixmap tankSprite;
    QPixmap rocketSprite;
    QPixmap pvoSprite;
    QPixmap treeSprite;
    QPixmap grassTexture;
    QPixmap asphaltTexture;

    // === ИГРОВЫЕ ОБЪЕКТЫ ===
    QVector<Target> targets;
    QVector<Rocket> rockets;
    QVector<HitMarker> hitMarkers;
    QVector<PvoBullet> pvoBullets;
    QVector<PlaneSkin> skins;
    QVector<GrassBlade> grass;
    QVector<QPointF> trees;
    Airfield airfield;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    AirplaneGame game;
    game.show();
    return app.exec();
}
