#include <QApplication>
#include <QWidget>
#include <QPainter>
#include <QTimer>
#include <QKeyEvent>
#include <QVector2D>
#include <QPixmap>
#include <QVector>
#include <QRectF>
#include <cmath>
#include <QFile>
#include <QTextStream>
#include <QFont>
#include <QDir>

// --- КОНСТАНТЫ ---
constexpr float WORLD_SIZE = 128000.0f;
constexpr int CHUNK_SIZE = 256;

// --- СТРУКТУРЫ ---
struct Target {
    float x, y;
    bool alive;
    int type;
    int hp;
    float angle;
    Target(float x_, float y_, int t) : x(x_), y(y_), alive(true), type(t), hp(3), angle(0) {}
};

struct HitMarker {
    float x, y;
    int timer;
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

struct EnemyPlane {
    float x, y;
    float vx, vy;
    float angle;
    int hp;
    int type;
    bool alive;
    int shootTimer;
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
    int speedBonus;
    int armorBonus;
    int bombBonus;
    float turnSpeed;
    bool owned;
    QString spriteFile;
    int flameOffsetX;
    int flameOffsetY1;
    int flameOffsetY2;
};

struct FieldZone {
    QRectF rect;
    QPixmap *texture;
};

// --- КЛАСС ИГРЫ ---
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
        resize(1280, 720);
        setMinimumSize(800, 600);
        setWindowTitle("AirSim");

        srand(time(nullptr));

        auto loadSafe = [](const QString &path, int w, int h) -> QPixmap {
            QPixmap pm(path);
            if (pm.isNull()) {
                QPixmap fallback(w, h);
                if (path.contains("corn")) fallback.fill(QColor(34, 139, 34));
                else if (path.contains("plowed")) fallback.fill(QColor(139, 69, 19));
                else if (path.contains("sunflower")) fallback.fill(QColor(255, 215, 0));
                else if (path.contains("grass")) fallback.fill(QColor(50, 205, 50));
                else if (path.contains("asphalt")) fallback.fill(Qt::gray);
                else fallback.fill(Qt::darkGray);

                QPainter p(&fallback);
                p.setPen(Qt::white);
                p.drawText(fallback.rect(), Qt::AlignCenter, path.section('/', -1).section('.', 0, 0));
                return fallback;
            }
            return pm.scaled(w, h, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        };

        forsageSprite = loadSafe(":/forsage.png", 40, 40);

        // Загрузка самолетов
        su27Sprite = loadSafe(":/plane.png", 90, 90);      // СУ-27
        mig29Sprite = loadSafe(":/plane7.png", 90, 90);    // МиГ-29
        su30Sprite = loadSafe(":/plane1.png", 90, 90);     // СУ-30
        su33Sprite = loadSafe(":/plane6.png", 90, 90);     // СУ-33
        su25Sprite = loadSafe(":/plane4.png", 85, 85);     // СУ-25
        su34Sprite = loadSafe(":/plane2.png", 130, 130);   // СУ-34
        su57Sprite = loadSafe(":/plane3.png", 90, 90);     // СУ-57
        mig31Sprite = loadSafe(":/plane5.png", 110, 110);  // МиГ-31

        eurofighterSprite = loadSafe(":/EF.png", 95, 95);
        f18Sprite = loadSafe(":/f18.png", 80, 80);

        grassTexture = loadSafe(":/grass_texture.png", 128, 128);
        asphaltTexture = loadSafe(":/asphalt.png", 128, 128);
        tankSprite = loadSafe(":/tank.png", 50, 40);
        AASprite = loadSafe(":/AA.png", 50, 50);
        treeSprite = loadSafe(":/tree.png", 80, 80);
        rocketSprite = loadSafe(":/rocket.png", 40, 30);

        cornTexture = loadSafe(":/corn.png", CHUNK_SIZE, CHUNK_SIZE);
        plowedTexture = loadSafe(":/plowed.png", CHUNK_SIZE, CHUNK_SIZE);
        sunflowerTexture = loadSafe(":/sunflower.png", CHUNK_SIZE, CHUNK_SIZE);

        // --- АНГАР (МиГ-29 добавлен вторым) ---
        skins.append({"СУ-27", 0, 2, 0, 0, 3.0f, true, ":/plane.png", -58, -5, 5});       // 0
        skins.append({"МиГ-29", 1500, 3, 5, 20, 3.5f, false, ":/plane7.png", -55, -5, 5}); // 1
        skins.append({"СУ-30", 2000, 3, 10, 0, 3.0f, false, ":/plane1.png", -61, -5, 5});  // 2
        skins.append({"СУ-33", 2500, 3, 15, 30, 3.5f, false, ":/plane6.png", -61, -5, 5}); // 3
        skins.append({"СУ-34", 3000, 3, 15, 40, 2.5f, false, ":/plane2.png", -75, -5, 5}); // 4
        skins.append({"СУ-25", 3500, 2, 25, 50, 2.5f, false, ":/plane4.png", -37, -5, 5}); // 5
        skins.append({"СУ-57", 5000, 5, 20, 60, 4.0f, false, ":/plane3.png", -55, -6, 6}); // 6
        skins.append({"МиГ-31", 10000, 10, 20, 70, 3.0f, false, ":/plane5.png", -75, -3, 3}); // 7

        loadMoney();
        loadHangar();

        // Генерация целей
        for (int i = 0; i < 10; i++) {
            targets.append(Target(1000 + rand() % 126000, 1000 + rand() % 126000, (rand() % 2 == 0) ? 0 : 2));
        }

        // Генерация врагов
        for (int i = 0; i < 10; i++) {
            enemies.append(EnemyPlane(1000 + rand() % 126000, 1000 + rand() % 126000, rand() % 2));
        }

        // Генерация деревьев
        for (int i = 0; i < 1000; i++) {
            trees.append(QPointF(1000 + rand() % 126000, 1000 + rand() % 126000));
        }

        // Генерация больших полей
        int numFields = 40;
        for (int i = 0; i < numFields; i++) {
            float x = 1000 + rand() % 120000;
            float y = 1000 + rand() % 120000;

            float w = 1500 + (rand() % 2500);
            float h = 1500 + (rand() % 2500);

            FieldZone zone;
            zone.rect = QRectF(x, y, w, h);

            int type = rand() % 3;
            if (type == 0) zone.texture = &cornTexture;
            else if (type == 1) zone.texture = &plowedTexture;
            else zone.texture = &sunflowerTexture;

            fieldZones.append(zone);
        }

        QTimer *timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &AirplaneGame::updateGame);
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
            for (int i = 0; i < skins.size(); i++) out << (skins[i].owned ? 1 : 0) << " ";
            out << "\n" << currentPlane;
        }
    }

    void loadHangar() {
        QFile file("hangar.txt");
        if (file.open(QIODevice::ReadOnly)) {
            QTextStream in(&file);
            for (int i = 0; i < skins.size(); i++) { int o; in >> o; skins[i].owned = (o==1); }
            in >> currentPlane;
            if (currentPlane >= skins.size()) currentPlane = 0;
        }
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        int w = width();
        int h = height();

        float camX = posX - w * 0.5f;
        float camY = posY - h * 0.5f;

        if (camX < 0) camX = 0; else if (camX > WORLD_SIZE - w) camX = WORLD_SIZE - w;
        if (camY < 0) camY = 0; else if (camY > WORLD_SIZE - h) camY = WORLD_SIZE - h;

        painter.translate(-camX, -camY);

        // Фон (Трава)
        if (!grassTexture.isNull()) {
            painter.setBrush(QBrush(grassTexture));
            painter.setPen(Qt::NoPen);
            painter.drawRect(camX, camY, w, h);
        }

        // Отрисовка больших полей
        for (const FieldZone &zone : fieldZones) {
            if (!zone.rect.intersects(QRectF(camX, camY, w, h))) continue;
            if (zone.texture && !zone.texture->isNull()) {
                painter.setBrush(QBrush(*zone.texture));
                painter.setPen(Qt::NoPen);
                painter.drawRect(zone.rect);
            }
        }

        // Деревья
        if (!treeSprite.isNull()) {
            for (const QPointF &tree : trees) {
                if (tree.x() < camX - 100 || tree.x() > camX + w + 100 ||
                    tree.y() < camY - 100 || tree.y() > camY + h + 100) continue;
                painter.drawPixmap(tree.x() - treeSprite.width()/2, tree.y() - treeSprite.height()/2, treeSprite);
            }
        }

        // Враги
        for (const EnemyPlane &e : enemies) {
            if (!e.alive) continue;
            if (e.x < camX - 200 || e.x > camX + w + 200 || e.y < camY - 200 || e.y > camY + h + 200) continue;

            painter.save();
            painter.translate(e.x, e.y);
            painter.rotate(e.angle + 90);
            const QPixmap *sprite = (e.type == 0) ? &eurofighterSprite : &f18Sprite;
            painter.drawPixmap(-sprite->width()/2, -sprite->height()/2, *sprite);

            if (e.hp < 3) {
                painter.setPen(Qt::NoPen); painter.setBrush(Qt::red);
                painter.drawRect(-15, -25, 30, 4);
                painter.setBrush(Qt::green); painter.drawRect(-15, -25, 10 * e.hp, 4);
            }
            painter.restore();
        }

        // Ракеты
        if (!rocketSprite.isNull()) {
            for (const Rocket &r : rockets) {
                if (r.x < camX - 50 || r.x > camX + w + 50 || r.y < camY - 50 || r.y > camY + h + 50) continue;
                painter.save();
                painter.translate(r.x, r.y);
                painter.rotate(atan2(r.vy, r.vx) * 180 / M_PI);
                painter.drawPixmap(-rocketSprite.width()/2, -rocketSprite.height()/2, rocketSprite);
                painter.restore();
            }
        }

        // Аэродром
        painter.save();
        painter.translate(airfield.x, airfield.y);
        if (!asphaltTexture.isNull()) {
            painter.setBrush(QBrush(asphaltTexture)); painter.setPen(Qt::NoPen);
            painter.drawRect(-300, -30, 1200, 80);
        }
        QColor lightColor = (fuel < 30 && blink && !onGround) ? Qt::red : Qt::yellow;
        painter.setBrush(lightColor); painter.setPen(Qt::NoPen);
        for (int i = -280; i <= 900; i += 45) {
            painter.drawEllipse(i, -45, 7, 7); painter.drawEllipse(i, 55, 7, 7);
        }
        painter.restore();

        // Цели
        for (const Target &t : targets) {
            if (!t.alive) continue;
            if (t.x < camX - 100 || t.x > camX + w + 100 || t.y < camY - 100 || t.y > camY + h + 100) continue;

            painter.save(); painter.translate(t.x, t.y);
            if (t.type == 0) {
                if (!tankSprite.isNull()) painter.drawPixmap(-tankSprite.width()/2, -tankSprite.height()/2, tankSprite);
            } else {
                if (!AASprite.isNull()) {
                    painter.drawPixmap(-AASprite.width()/2, -AASprite.height()/2, AASprite);
                    painter.setPen(QPen(Qt::black, 3));
                    painter.drawLine(0, 0, cos(t.angle)*25, sin(t.angle)*25);
                }
            }
            painter.restore();

            if (t.hp < 3) {
                float yOffset = (t.type == 2) ? -25.0f : -20.0f;
                painter.setPen(Qt::NoPen); painter.setBrush(Qt::red);
                painter.drawRect(t.x - 12, t.y + yOffset, 24, 4);
                painter.setBrush(Qt::green);
                painter.drawRect(t.x - 12, t.y + yOffset, 8 * t.hp, 4);
            }
        }

        // Пули
        painter.setBrush(Qt::yellow); painter.setPen(Qt::NoPen);
        for (const AABullet &b : AABullets) {
            if (b.x < camX || b.x > camX + w || b.y < camY || b.y > camY + h) continue;
            painter.drawEllipse(b.x - 2, b.y - 2, 4, 4);
        }

        // Маркеры
        for (const HitMarker &hm : hitMarkers) {
            float p = hm.timer / 10.0f;
            painter.setPen(QPen(QColor(255, 100, 0, 255 * p), 2));
            painter.drawLine(hm.x - 8, hm.y - 8, hm.x + 8, hm.y + 8);
            painter.drawLine(hm.x + 8, hm.y - 8, hm.x - 8, hm.y + 8);
        }

        // Игрок
        painter.save();
        painter.translate(posX, posY);
        painter.rotate(angle + 90);

        const QPixmap *curSprite = &su27Sprite;
        switch (currentPlane) {
            case 0: curSprite = &su27Sprite; break;
            case 1: if (!mig29Sprite.isNull()) curSprite = &mig29Sprite; break;
            case 2: if (!su30Sprite.isNull()) curSprite = &su30Sprite; break;
            case 3: if (!su33Sprite.isNull()) curSprite = &su33Sprite; break;
            case 4: if (!su34Sprite.isNull()) curSprite = &su34Sprite; break;
            case 5: if (!su25Sprite.isNull()) curSprite = &su25Sprite; break;
            case 6: if (!su57Sprite.isNull()) curSprite = &su57Sprite; break;
            case 7: if (!mig31Sprite.isNull()) curSprite = &mig31Sprite; break;
        }

        painter.drawPixmap(-curSprite->width()/2, -curSprite->height()/2, *curSprite);
        painter.restore();

        // Форсаж
        if (forsageAlpha > 0.01f && !forsageSprite.isNull()) {
            painter.save();
            painter.translate(posX, posY);
            painter.rotate(angle);
            painter.setOpacity(forsageAlpha);
            const PlaneSkin &skin = skins[currentPlane];
            float fw = forsageSprite.width() * 0.5f;
            float fh = forsageSprite.height() * 0.5f;

            painter.save(); painter.translate(skin.flameOffsetX, skin.flameOffsetY1); painter.rotate(-90);
            painter.drawPixmap(-fw, -fh, forsageSprite); painter.restore();

            painter.save(); painter.translate(skin.flameOffsetX, skin.flameOffsetY2); painter.rotate(-90);
            painter.drawPixmap(-fw, -fh, forsageSprite); painter.restore();

            painter.setOpacity(1.0f); painter.restore();
        }

        painter.resetTransform();
        drawRadar(painter);

        if (landingMenuOpen) {
            painter.fillRect(rect(), QColor(0, 0, 0, 200));
            painter.setPen(Qt::white); painter.setFont(QFont("Arial", 24, QFont::Bold));
            painter.drawText(rect().adjusted(0, -100, 0, 0), Qt::AlignHCenter, "ВЫБЕРИТЕ ДЕЙСТВИЕ");
            painter.setFont(QFont("Arial", 18));

            QString opt1 = (landingChoice == 0) ? "> СВОБОДНЫЙ ПОЛЁТ" : "   СВОБОДНЫЙ ПОЛЁТ";
            QString opt2 = (landingChoice == 1) ? "> НОВАЯ МИССИЯ" : "   НОВАЯ МИССИЯ";

            painter.setPen(landingChoice == 0 ? Qt::green : Qt::white);
            painter.drawText(rect().adjusted(0, 0, 0, 0), Qt::AlignHCenter, opt1);
            painter.setPen(landingChoice == 1 ? Qt::green : Qt::white);
            painter.drawText(rect().adjusted(0, 50, 0, 0), Qt::AlignHCenter, opt2);

            painter.setPen(Qt::gray); painter.setFont(QFont("Arial", 14));
            painter.drawText(rect().adjusted(0, 150, 0, 0), Qt::AlignHCenter, "↑/↓ выбор, ENTER подтвердить");
        }

        if (hydraulicFailure && hydraulicTimer > 0) {
            painter.setPen(Qt::red); painter.setFont(QFont("Arial", 24, QFont::Bold));
            painter.drawText(rect().adjusted(0, -200, 0, 0), Qt::AlignHCenter, "! ОТКАЗ ГИДРОСИСТЕМ !");
            painter.setFont(QFont("Arial", 14)); painter.setPen(Qt::white);
            painter.drawText(rect().adjusted(0, -150, 0, 0), Qt::AlignHCenter, "управление затруднено");
            hydraulicTimer--;
        }

        if (shopOpen) {
            drawShop(painter);
        } else {
            painter.setPen(Qt::white);
            painter.setFont(QFont("Arial", 12));

            int thrustPercent = static_cast<int>((thrust / (5.0f + skins[currentPlane].speedBonus)) * 100);
            if (thrustPercent > 100) thrustPercent = 100;

            painter.drawText(10, 30, "топливо: " + QString::number(fuel, 'f', 1) + "%");
            painter.drawText(10, 50, "бомбы: " + QString::number(bombs));
            painter.drawText(10, 70, "деньги: " + QString::number(money));
            painter.drawText(10, 90, "броня: " + QString::number(armor));
            painter.drawText(10, 110, "тяга: " + QString::number(thrustPercent) + "%");

            if (onGround) {
                painter.setPen(Qt::green);
                painter.drawText(10, 140, "на земле - заправка | M магазин");
            } else {
                painter.setPen(Qt::white);
                painter.drawText(10, 140, "SPACE - бомба, R - ракета");
            }

            if (fuel < 30 && !onGround) {
                painter.setPen(blink ? Qt::red : Qt::white);
                painter.setFont(QFont("Arial", 14, QFont::Bold));
                painter.drawText(width()/2 - 60, 100, "МАЛО ТОПЛИВА!");
            }

            if (!onGround) drawAirfieldArrow(painter);
        }
    }

    void drawAirfieldArrow(QPainter &p) {
        float dx = airfield.x - posX, dy = airfield.y - posY;
        float dist = sqrt(dx*dx + dy*dy);
        float ang = atan2(dy, dx) * 180 / M_PI;
        p.save(); p.translate(100, height() - 100);
        p.setPen(QPen(Qt::white, 2)); p.setBrush(QColor(0, 0, 0, 150));
        p.drawEllipse(-30, -30, 60, 60);
        p.save(); p.rotate(ang - angle);
        p.setBrush(Qt::green); p.setPen(Qt::NoPen);
        p.drawPolygon(QPolygon() << QPoint(0, -20) << QPoint(-8, 5) << QPoint(8, 5));
        p.restore();
        p.setPen(Qt::white); p.setFont(QFont("Arial", 10));
        p.drawText(-20, 40, QString::number((int)dist) + " м");
        p.setPen(Qt::green); p.setFont(QFont("Arial", 12, QFont::Bold));
        p.drawText(-5, -35, "А"); p.restore();
    }

    void drawRadar(QPainter &p) {
        p.save(); p.translate(width() - 180, 80);
        p.setBrush(QColor(0, 30, 0, 200)); p.setPen(QPen(Qt::green, 2));
        p.drawEllipse(-60, -60, 120, 120);
        p.setPen(QPen(Qt::green, 1, Qt::DotLine));
        p.drawLine(-60, 0, 60, 0); p.drawLine(0, -60, 0, 60);
        static float scan = 0; scan += 0.05f; if (scan > 6.28f) scan -= 6.28f;
        p.setPen(QPen(Qt::green, 2));
        p.drawLine(0, 0, cos(scan) * 55, sin(scan) * 55);

        float rSq = 70000.0f * 70000.0f, eSq = 20000.0f * 20000.0f;
        for (const Target &t : targets) {
            if (!t.alive) continue;
            float dx = t.x - posX, dy = t.y - posY;
            if (dx*dx + dy*dy < rSq) {
                float s = 50.0f / 70000.0f;
                p.setBrush(t.type == 2 ? Qt::red : Qt::yellow); p.setPen(Qt::NoPen);
                p.drawEllipse(dx*s - 3, dy*s - 3, 6, 6);
            }
        }
        for (const EnemyPlane &e : enemies) {
            if (!e.alive) continue;
            float dx = e.x - posX, dy = e.y - posY;
            if (dx*dx + dy*dy < eSq) {
                float s = 50.0f / 20000.0f;
                p.setBrush(Qt::magenta); p.setPen(Qt::NoPen);
                p.drawEllipse(dx*s - 3, dy*s - 3, 6, 6);
            }
        }
        p.setBrush(Qt::cyan); p.drawEllipse(-4, -4, 8, 8); p.restore();
    }

    void drawShop(QPainter &p) {
        p.fillRect(rect(), QColor(0, 0, 0, 220));
        p.setPen(Qt::white); p.setFont(QFont("Arial", 18, QFont::Bold));
        p.drawText(rect().adjusted(0, 20, 0, 0), Qt::AlignHCenter, "Ангар");
        p.setFont(QFont("Arial", 12));
        for (int i = 0; i < skins.size(); i++) {
            int y = 130 + i * 90;
            p.setPen(i == selectedSkin ? QPen(Qt::green, 3) : QPen(Qt::gray, 1));
            p.setBrush(i == selectedSkin ? QColor(0, 70, 0, 70) : Qt::NoBrush);
            p.drawRect(150, y - 35, 700, 80);

            if (skins[i].owned || i == 0) {
                QPixmap mini = QPixmap(skins[i].spriteFile).scaled(60, 50, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                if (mini.isNull()) { p.setBrush(Qt::gray); p.drawRect(160, y - 25, 60, 50); }
                else p.drawPixmap(160, y - 25, mini);
            } else {
                p.setBrush(Qt::darkGray); p.drawRect(160, y - 25, 60, 50);
                p.setPen(Qt::white); p.drawText(180, y + 10, "?");
            }

            p.setPen(Qt::white); p.setFont(QFont("Arial", 14, QFont::Bold));
            p.drawText(240, y - 10, skins[i].name);
            p.setFont(QFont("Arial", 11));

            QString stats = "+" + QString::number(skins[i].speedBonus) + " скор\n" +
                            "+" + QString::number(skins[i].armorBonus) + " броня\n" +
                            "+" + QString::number(skins[i].bombBonus) + " бомб\n" +
                            QString::number(skins[i].turnSpeed) + " вращ";

            p.drawText(240, y + 30, stats);

            if (skins[i].owned) {
                p.setPen(Qt::green);
                p.drawText(680, y, (i == currentPlane) ? "выбрано" : "куплено");
            } else {
                p.setPen(Qt::yellow);
                p.drawText(550, y, QString::number(skins[i].price) + " $");
            }
        }
        p.setPen(Qt::white); p.setFont(QFont("Arial", 12));
        p.drawText(rect().adjusted(0, 50, 0, 0), Qt::AlignHCenter, "M - выход, ↑/↓ - выбор, ENTER - купить");
    }

    void keyPressEvent(QKeyEvent *e) override {
        if (e->key() == Qt::Key_Escape) qApp->quit();

        if (shopOpen) {
            if (e->key() == Qt::Key_M) { shopOpen = false; return; }
            if (e->key() == Qt::Key_Up) { selectedSkin = (selectedSkin - 1 + skins.size()) % skins.size(); update(); return; }
            if (e->key() == Qt::Key_Down) { selectedSkin = (selectedSkin + 1) % skins.size(); update(); return; }
            if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
                if (!skins[selectedSkin].owned) {
                    if (money >= skins[selectedSkin].price) {
                        money -= skins[selectedSkin].price;
                        skins[selectedSkin].owned = true; currentPlane = selectedSkin;
                        saveMoney(); saveHangar();
                    }
                } else { currentPlane = selectedSkin; saveHangar(); }
                update(); return;
            }
            return;
        }

        if (e->key() == Qt::Key_W) keyW = true;
        if (e->key() == Qt::Key_S) keyS = true;
        if (e->key() == Qt::Key_A) keyA = true;
        if (e->key() == Qt::Key_D) keyD = true;
        if (e->key() == Qt::Key_M && onGround) { shopOpen = true; selectedSkin = currentPlane; }

        if (e->key() == Qt::Key_R && !onGround && money >= 10) {
            money -= 10; saveMoney();
            float rad = angle * M_PI / 180.0f;
            EnemyPlane *ne = nullptr; Target *nt = nullptr; float minD = 999999999.0f;
            for (EnemyPlane &en : enemies) {
                if (!en.alive) continue;
                float dx = posX - en.x, dy = posY - en.y, d = dx*dx + dy*dy;
                if (d < minD) { minD = d; ne = &en; }
            }
            if (!ne) {
                for (Target &t : targets) {
                    if (!t.alive) continue;
                    float dx = posX - t.x, dy = posY - t.y, d = dx*dx + dy*dy;
                    if (d < minD) { minD = d; nt = &t; }
                }
            }
            Rocket r;
            r.x = posX + cos(rad) * 50; r.y = posY + sin(rad) * 50;
            r.vx = cos(rad) * 40.0f; r.vy = sin(rad) * 40.0f;
            r.timer = 300; r.target = nt; r.enemyTarget = ne; r.isHoming = true;
            rockets.append(r);
        }

        if (landingMenuOpen) {
            if (e->key() == Qt::Key_Up) { landingChoice = 0; update(); return; }
            if (e->key() == Qt::Key_Down) { landingChoice = 1; update(); return; }
            if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
                resetGame(landingChoice == 1); landingMenuOpen = false; update(); return;
            }
            return;
        }
    }

    void keyReleaseEvent(QKeyEvent *e) override {
        if (shopOpen) return;
        if (e->key() == Qt::Key_W) keyW = false;
        if (e->key() == Qt::Key_S) keyS = false;
        if (e->key() == Qt::Key_A) keyA = false;
        if (e->key() == Qt::Key_D) keyD = false;
        if (e->key() == Qt::Key_Space && bombs > 0 && !onGround) dropBomb();
    }

private slots:
    void updateGame() {
        if (shopOpen) return;

        if (thrust > 3.0f && !onGround) forsageAlpha = qMin(forsageAlpha + 0.05f, 1.0f);
        else forsageAlpha = qMax(forsageAlpha - 0.05f, 0.0f);

        if (armor <= 30 && armor > 0 && !hydraulicFailure) { hydraulicFailure = true; hydraulicTimer = 120; }

        int spB = hydraulicFailure ? 0 : skins[currentPlane].speedBonus;
        float tsV = hydraulicFailure ? 1.0f : skins[currentPlane].turnSpeed;

        if (keyW) thrust = qMin(thrust + 0.1f, 5.0f + spB);
        if (keyS) thrust = qMax(thrust - 0.1f, 0.0f);
        if (keyA) angle -= tsV;
        if (keyD) angle += tsV;
        angle = fmod(angle, 360.0f); if (angle < 0) angle += 360.0f;

        if (thrust > 0 && fuel > 0 && !onGround) { fuel -= 0.01f; if (fuel < 0) fuel = 0; }
        if (fuel < 20 && fuel > 0 && !onGround && !fuelWarningPlayed) { QApplication::beep(); fuelWarningPlayed = true; }
        if (fuel >= 20 || onGround) fuelWarningPlayed = false;

        float rad = angle * M_PI / 180.0f, cA = cos(rad), sA = sin(rad);
        if (fuel > 0) {
            velocity.setX(velocity.x() + cA * thrust * 0.05f);
            velocity.setY(velocity.y() + sA * thrust * 0.05f);
        }
        velocity *= onGround ? 0.95f : 0.99f;
        posX += velocity.x(); posY += velocity.y();
        if (posX < 0) posX = 0; else if (posX > WORLD_SIZE) posX = WORLD_SIZE;
        if (posY < 0) posY = 0; else if (posY > WORLD_SIZE) posY = WORLD_SIZE;

        for (Target &t : targets) {
            if (!t.alive || t.type != 2) continue;
            float dx = posX - t.x, dy = posY - t.y, dSq = dx*dx + dy*dy;
            if (dSq < 360000.0f) {
                t.angle = atan2(dy, dx);
                if (rand() % 30 == 0) {
                    AABullet b; b.x = t.x; b.y = t.y;
                    float sp = 100.0f;
                    b.vx = cos(t.angle) * sp; b.vy = sin(t.angle) * sp;
                    b.timer = 100; AABullets.append(b);
                }
            }
        }

        for (int i = AABullets.size() - 1; i >= 0; i--) {
            AABullet &b = AABullets[i];
            b.x += b.vx; b.y += b.vy; b.timer--;
            float dx = posX - b.x, dy = posY - b.y;
            if (dx*dx + dy*dy < 225.0f && !onGround) {
                armor -= 10; hitMarkers.append({posX, posY, 10});
                AABullets.removeAt(i);
                if (armor <= 0) resetPlayer(skins[currentPlane].armorBonus);
                continue;
            }
            if (b.timer <= 0 || b.x < 0 || b.x > WORLD_SIZE || b.y < 0 || b.y > WORLD_SIZE) AABullets.removeAt(i);
        }

        for (int i = AABullets.size() - 1; i >= 0; i--) {
            AABullet &b = AABullets[i];
            for (EnemyPlane &e : enemies) {
                if (!e.alive) continue;
                float dx = b.x - e.x, dy = b.y - e.y;
                if (dx*dx + dy*dy < 225.0f) {
                    e.hp--; hitMarkers.append({e.x, e.y, 10});
                    if (e.hp <= 0) e.alive = false;
                    AABullets.removeAt(i); break;
                }
            }
        }

        for (EnemyPlane &e : enemies) {
            if (!e.alive) continue;
            float dx = posX - e.x, dy = posY - e.y, dSq = dx*dx + dy*dy;
            if (dSq < 1440000.0f) {
                float tAng = atan2(dy, dx) * 180 / M_PI;
                float diff = tAng - e.angle;
                while (diff > 180) diff -= 360; while (diff < -180) diff += 360;
                if (diff > 5.0f) diff = 5.0f; if (diff < -5.0f) diff = -5.0f;
                e.angle += diff;
                float rE = e.angle * M_PI / 180.0f, cE = cos(rE), sE = sin(rE);
                e.vx += cE * 0.1f; e.vy += sE * 0.1f;
                float spSq = e.vx*e.vx + e.vy*e.vy;
                if (spSq > 72.25f) { float s = 8.5f / sqrt(spSq); e.vx *= s; e.vy *= s; }
                e.shootTimer++;
                if (e.shootTimer > 45 && dSq < 490000.0f) {
                    e.shootTimer = 0;
                    Rocket r; r.x = e.x + cE * 30; r.y = e.y + sE * 30;
                    r.vx = cE * 40.0f; r.vy = sE * 40.0f; r.timer = 200;
                    r.isHoming = true; enemyRockets.append(r);
                }
            } else {
                e.angle += 0.5f;
                float rE = e.angle * M_PI / 180.0f, cE = cos(rE), sE = sin(rE);
                e.vx += cE * 0.05f; e.vy += sE * 0.05f;
                float spSq = e.vx*e.vx + e.vy*e.vy;
                if (spSq > 9.0f) { float s = 3.0f / sqrt(spSq); e.vx *= s; e.vy *= s; }
            }
            e.x += e.vx; e.y += e.vy;
            e.vx *= 0.99f; e.vy *= 0.99f;
            if (e.x < 0) { e.x = WORLD_SIZE; e.vx = 0; } else if (e.x > WORLD_SIZE) { e.x = 0; e.vx = 0; }
            if (e.y < 0) { e.y = WORLD_SIZE; e.vy = 0; } else if (e.y > WORLD_SIZE) { e.y = 0; e.vy = 0; }
        }

        for (int i = rockets.size() - 1; i >= 0; i--) {
            Rocket &r = rockets[i];
            if (r.isHoming) {
                float tx = 0, ty = 0; bool has = false;
                if (r.enemyTarget && r.enemyTarget->alive) { tx = r.enemyTarget->x; ty = r.enemyTarget->y; has = true; }
                else if (r.target && r.target->alive) { tx = r.target->x; ty = r.target->y; has = true; }
                if (has) {
                    float dx = tx - r.x, dy = ty - r.y, dSq = dx*dx + dy*dy;
                    if (dSq > 400.0f) {
                        float tAng = atan2(dy, dx);
                        float cAng = atan2(r.vy, r.vx);
                        float diff = tAng - cAng;
                        while (diff > M_PI) diff -= 2*M_PI; while (diff < -M_PI) diff += 2*M_PI;
                        cAng += diff * 0.05f;
                        float sp = sqrt(r.vx*r.vx + r.vy*r.vy);
                        r.vx = cos(cAng) * sp; r.vy = sin(cAng) * sp;
                    }
                } else r.isHoming = false;
            }
            r.x += r.vx; r.y += r.vy; r.timer--;
            bool hit = false;
            for (EnemyPlane &e : enemies) {
                if (!e.alive) continue;
                float dx = r.x - e.x, dy = r.y - e.y;
                if (dx*dx + dy*dy < 400.0f) {
                    e.hp -= 3; hitMarkers.append({e.x, e.y, 15});
                    if (e.hp <= 0) { e.alive = false; money += 50; saveMoney(); }
                    rockets.removeAt(i); hit = true; break;
                }
            }
            if (hit) continue;
            for (Target &t : targets) {
                if (!t.alive) continue;
                float dx = r.x - t.x, dy = r.y - t.y;
                if (dx*dx + dy*dy < 400.0f) {
                    t.hp -= 5; hitMarkers.append({t.x, t.y, 15});
                    if (t.hp <= 0) { t.alive = false; money += 10; saveMoney(); }
                    rockets.removeAt(i); hit = true; break;
                }
            }
            if (hit) continue;
            if (r.timer <= 0 || r.x < 0 || r.x > WORLD_SIZE || r.y < 0 || r.y > WORLD_SIZE) rockets.removeAt(i);
        }

        for (int i = enemyRockets.size() - 1; i >= 0; i--) {
            Rocket &r = enemyRockets[i];
            if (r.isHoming) {
                float dx = posX - r.x, dy = posY - r.y, dSq = dx*dx + dy*dy;
                if (dSq > 400.0f) {
                    float tAng = atan2(dy, dx);
                    float cAng = atan2(r.vy, r.vx);
                    float diff = tAng - cAng;
                    while (diff > M_PI) diff -= 2*M_PI; while (diff < -M_PI) diff += 2*M_PI;
                    cAng += diff * 0.05f;
                    float sp = sqrt(r.vx*r.vx + r.vy*r.vy);
                    r.vx = cos(cAng) * sp; r.vy = sin(cAng) * sp;
                }
            }
            r.x += r.vx; r.y += r.vy; r.timer--;
            float dx = posX - r.x, dy = posY - r.y;
            if (dx*dx + dy*dy < 400.0f && !onGround) {
                armor -= 20; hitMarkers.append({posX, posY, 15});
                enemyRockets.removeAt(i);
                if (armor <= 0) resetPlayer(skins[currentPlane].armorBonus);
                continue;
            }
            if (r.timer <= 0 || r.x < 0 || r.x > WORLD_SIZE || r.y < 0 || r.y > WORLD_SIZE) enemyRockets.removeAt(i);
        }

        for (int i = hitMarkers.size() - 1; i >= 0; i--) {
            hitMarkers[i].timer--;
            if (hitMarkers[i].timer <= 0) hitMarkers.removeAt(i);
        }

        float dx = posX - airfield.x, dy = posY - airfield.y;
        bool wasG = onGround;
        onGround = (dx*dx + dy*dy < 360000.0f && velocity.length() < 3.0f);
        if (onGround && !wasG) {
            fuel = 100; bombs = 50 + skins[currentPlane].bombBonus;
            armor = 100 + skins[currentPlane].armorBonus; money += 50;
            hydraulicFailure = false; hydraulicTimer = 0;
            saveMoney(); landingMenuOpen = true; landingChoice = 0;
        }
    }

    void dropBomb() {
        bombs--;
        for (Target &t : targets) {
            if (!t.alive) continue;
            float dx = posX - t.x, dy = posY - t.y;
            if (dx*dx + dy*dy < 3600.0f) {
                t.hp--; hitMarkers.append({t.x, t.y, 10});
                if (t.hp <= 0) {
                    t.alive = false; money += 20 + (t.type == 2 ? 30 : 0); saveMoney();
                }
            }
        }
    }

    void resetPlayer(int ab) {
        posX = 4000; posY = 4000; armor = 100 + ab;
        money = 0; bombs = 50; fuel = 100;
        hydraulicFailure = false; saveMoney();
    }

    void resetGame(bool nm) {
        fuel = 100; bombs = 50 + skins[currentPlane].bombBonus;
        armor = 100 + skins[currentPlane].armorBonus; money += 50; saveMoney();
        if (nm) {
            targets.clear(); enemies.clear(); trees.clear();
            fieldZones.clear();

            for (int i = 0; i < 10; i++) targets.append(Target(1000 + rand() % 126000, 1000 + rand() % 126000, (rand()%2==0)?0:2));
            for (int i = 0; i < 10; i++) enemies.append(EnemyPlane(1000 + rand() % 126000, 1000 + rand() % 126000, rand()%2));
            for (int i = 0; i < 1000; i++) trees.append(QPointF(1000 + rand() % 126000, 1000 + rand() % 126000));

            int numFields = 40;
            for (int i = 0; i < numFields; i++) {
                float x = 1000 + rand() % 120000;
                float y = 1000 + rand() % 120000;
                float w = 1500 + (rand() % 2500);
                float h = 1500 + (rand() % 2500);
                FieldZone zone;
                zone.rect = QRectF(x, y, w, h);
                int type = rand() % 3;
                if (type == 0) zone.texture = &cornTexture;
                else if (type == 1) zone.texture = &plowedTexture;
                else zone.texture = &sunflowerTexture;
                fieldZones.append(zone);
            }
        }
    }

    void loadMoney() {
        QFile file("money.txt");
        if (file.open(QIODevice::ReadOnly)) { QTextStream in(&file); in >> money; }
    }

    void saveMoney() {
        QFile file("money.txt");
        if (file.open(QIODevice::WriteOnly)) { QTextStream out(&file); out << money; }
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

    bool landingMenuOpen;
    int landingChoice;
    bool hydraulicFailure;
    int hydraulicTimer;
    float forsageAlpha;

    QPixmap su27Sprite, mig29Sprite, su30Sprite, su33Sprite, su25Sprite, su34Sprite, su57Sprite, mig31Sprite;
    QPixmap tankSprite, rocketSprite, AASprite, treeSprite, grassTexture, asphaltTexture;
    QPixmap eurofighterSprite, f18Sprite, forsageSprite;

    QPixmap cornTexture, plowedTexture, sunflowerTexture;

    QVector<Target> targets;
    QVector<Rocket> rockets;
    QVector<Rocket> enemyRockets;
    QVector<HitMarker> hitMarkers;
    QVector<AABullet> AABullets;
    QVector<PlaneSkin> skins;
    QVector<QPointF> trees;
    QVector<EnemyPlane> enemies;

    QVector<FieldZone> fieldZones;

    Airfield airfield;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    AirplaneGame game;
    game.show();
    return app.exec();
}
