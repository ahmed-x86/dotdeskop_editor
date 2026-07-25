#include <QApplication>
#include <QWidget>
#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QRadialGradient>
#include <QTimer>
#include <QPushButton>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QFrame>
#include <QIcon>
#include <QPixmap>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QRegularExpression>
#include <QFont>
#include <QFontMetrics>
#include <QSet>
#include <QEasingCurve>
#include <QSizePolicy>
#include <QMenu>
#include <QAction>
#include <QClipboard>
#include <QGuiApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QProcess>
#include <algorithm>

namespace Mocha {
    static const QColor Base      (0x1e, 0x1e, 0x2e);   // #1e1e2e
    static const QColor Surface   (0x31, 0x32, 0x44);   // #313244
    static const QColor Text      (0xcd, 0xd6, 0xf4);   // #cdd6f4
    static const QColor Subtext   (0xa6, 0xad, 0xc8);   // #a6adc8
    static const QColor Overlay   (0x58, 0x5b, 0x70);   // #585b70
    static const QColor Hover     (0x45, 0x47, 0x5a);   // #45475a
    static const QColor Accent    (0xcb, 0xa6, 0xf7);   // #cba6f7
    static inline QColor Glow() {
        QColor c = Accent;
        c.setAlpha(30);
        return c;
    }
}
enum class ViewMode { Grid, List };
struct DesktopEntry {
    QString name;
    QString genericName;
    QString iconName;
    QString exec;
    QString keywords;
    QString filePath;

    bool matches(const QString &query) const {
        if (query.isEmpty()) return true;
        return name.contains(query, Qt::CaseInsensitive)
            || genericName.contains(query, Qt::CaseInsensitive)
            || exec.contains(query, Qt::CaseInsensitive)
            || keywords.contains(query, Qt::CaseInsensitive);
    }
};


class RippleButton : public QPushButton {
    Q_OBJECT
    Q_PROPERTY(qreal rippleRadius  READ rippleRadius  WRITE setRippleRadius)
    Q_PROPERTY(qreal rippleOpacity READ rippleOpacity WRITE setRippleOpacity)

public:
    explicit RippleButton(const QString &text = {}, QWidget *parent = nullptr)
        : QPushButton(text, parent)
    {
        setCursor(Qt::PointingHandCursor);
    }

    qreal rippleRadius()  const { return m_radius;  }
    qreal rippleOpacity() const { return m_opacity;  }
    void  setRippleRadius (qreal v) { m_radius  = v; update(); }
    void  setRippleOpacity(qreal v) { m_opacity = v; update(); }

protected:
    void mousePressEvent(QMouseEvent *e) override {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        m_center = e->position();
#else
        m_center = e->localPos();
#endif
        qreal maxDist = 0;
        const QList<QPointF> corners = {
            {0.0, 0.0},
            {qreal(width()), 0.0},
            {0.0, qreal(height())},
            {qreal(width()), qreal(height())}
        };
        for (const auto &c : corners) {
            qreal d = QLineF(m_center, c).length();
            if (d > maxDist) maxDist = d;
        }

        auto *rAnim = new QPropertyAnimation(this, "rippleRadius", this);
        rAnim->setDuration(520);
        rAnim->setStartValue(0.0);
        rAnim->setEndValue(maxDist);
        rAnim->setEasingCurve(QEasingCurve::OutCubic);

        auto *oAnim = new QPropertyAnimation(this, "rippleOpacity", this);
        oAnim->setDuration(520);
        oAnim->setStartValue(0.38);
        oAnim->setEndValue(0.0);
        oAnim->setEasingCurve(QEasingCurve::OutCubic);

        auto *group = new QParallelAnimationGroup(this);
        group->addAnimation(rAnim);
        group->addAnimation(oAnim);
        connect(group, &QAbstractAnimation::finished, group, &QObject::deleteLater);
        group->start();

        QPushButton::mousePressEvent(e);
    }

    void paintEvent(QPaintEvent *e) override {
        QPushButton::paintEvent(e);

        if (m_opacity > 0.0 && m_radius > 0.0) {
            QPainter p(this);
            p.setRenderHint(QPainter::Antialiasing);
            p.setClipRect(rect());
            QColor rc = Mocha::Accent;
            rc.setAlphaF(m_opacity);
            p.setBrush(rc);
            p.setPen(Qt::NoPen);
            p.drawEllipse(m_center, m_radius, m_radius);
        }
    }

private:
    QPointF m_center;
    qreal   m_radius  = 0;
    qreal   m_opacity = 0;
};


class OptionsButton : public QPushButton {
    Q_OBJECT

public:
    explicit OptionsButton(QWidget *parent = nullptr) : QPushButton(parent) {
        setText(QStringLiteral("\u22EE")); // vertical ellipsis ⋮
        setFixedSize(28, 28);
        setCursor(Qt::PointingHandCursor);
        setFlat(true);
        setStyleSheet(QStringLiteral(
            "QPushButton {"
            "  background: transparent;"
            "  color: #a6adc8;"
            "  border: none;"
            "  border-radius: 6px;"
            "  font-size: 18px;"
            "  font-weight: bold;"
            "  padding: 0px;"
            "}"
            "QPushButton:hover {"
            "  background: #45475a;"
            "  color: #cdd6f4;"
            "}"
            "QPushButton:pressed {"
            "  background: #585b70;"
            "}"
            "QPushButton::menu-indicator {"
            "  image: none;"
            "}"
        ));
    }
};


class ElidedLabel : public QWidget {
    Q_OBJECT
public:
    explicit ElidedLabel(QWidget *parent = nullptr) : QWidget(parent) {
        setAttribute(Qt::WA_TransparentForMouseEvents); // Pass hover/clicks to AppCard
    }

    void setText(const QString &text) {
        m_text = text;
        setToolTip(text);
        update();
    }

    void setTextColor(const QColor &c) {
        m_color = c;
        update();
    }

    void setAlignment(Qt::Alignment al) {
        m_align = al;
        update();
    }

    QSize sizeHint() const override {
        QFontMetrics fm(font());
        return {fm.horizontalAdvance(m_text), fm.height()};
    }

    QSize minimumSizeHint() const override {
        QFontMetrics fm(font());
        // Extremely small minimum width so the layout engine shrinks it dynamically
        return {10, fm.height()};
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::TextAntialiasing);
        p.setFont(font());
        p.setPen(m_color);
        QFontMetrics fm(font());
        QString elided = fm.elidedText(m_text, Qt::ElideRight, width());
        p.drawText(rect(), m_align | Qt::AlignVCenter, elided);
    }

private:
    QString m_text;
    QColor m_color = Qt::white;
    Qt::Alignment m_align = Qt::AlignLeft;
};


// =============================================================================
//  AppCard  —  dual-mode card (grid / list) for a .desktop entry
// =============================================================================
class AppCard : public QFrame {
    Q_OBJECT

public:
    AppCard(const DesktopEntry &entry, const QIcon &icon,
            ViewMode mode, QWidget *parent = nullptr)
        : QFrame(parent), m_entry(entry), m_icon(icon), m_mode(mode)
    {
        setCursor(Qt::PointingHandCursor);
        setAttribute(Qt::WA_Hover, true);
        buildLayout();
    }

    const DesktopEntry &entry() const { return m_entry; }

    void setViewMode(ViewMode mode) {
        if (m_mode == mode) return;
        m_mode = mode;
        deleteChildren();
        buildLayout();
        update();
    }

protected:
    bool event(QEvent *e) override {
        switch (e->type()) {
        case QEvent::Enter:
        case QEvent::HoverEnter:  m_hovered = true;  update(); break;
        case QEvent::Leave:
        case QEvent::HoverLeave:  m_hovered = false; update(); break;
        default: break;
        }
        return QFrame::event(e);
    }

    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        const QColor bg = m_hovered ? Mocha::Hover : Mocha::Surface;
        p.setBrush(bg);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 14, 14);

        if (m_hovered) {
            QPen pen(Mocha::Accent);
            pen.setWidthF(1.4);
            p.setPen(pen);
            p.setBrush(Qt::NoBrush);
            p.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 14, 14);
        }
    }

private:
    DesktopEntry m_entry;
    QIcon        m_icon;
    ViewMode     m_mode;
    bool         m_hovered = false;

    // Recursive layout clearing to fix ghost widgets
    void clearLayout(QLayout *lay) {
        if (!lay) return;
        while (QLayoutItem *item = lay->takeAt(0)) {
            if (QWidget *w = item->widget()) {
                delete w;
            }
            if (QLayout *childLay = item->layout()) {
                clearLayout(childLay);
            }
            delete item;
        }
    }

    void deleteChildren() {
        if (layout()) {
            clearLayout(layout());
            delete layout();
        }
    }

    void buildLayout() {
        const int iconSz = (m_mode == ViewMode::Grid) ? 48 : 36;

        // ---- Icon pixmap ----
        QPixmap pix = m_icon.pixmap(iconSz, iconSz);
        if (pix.isNull())
            pix = QIcon::fromTheme(QStringLiteral("application-x-executable")).pixmap(iconSz, iconSz);

        auto *iconLbl = new QLabel(this);
        iconLbl->setPixmap(pix);
        iconLbl->setAlignment(Qt::AlignCenter);
        iconLbl->setFixedSize(iconSz + 4, iconSz + 4);
        iconLbl->setStyleSheet(QStringLiteral("background:transparent;"));

        // ---- Name label (Custom Elided) ----
        auto *nameLbl = new ElidedLabel(this);
        QFont nf;
        nf.setPointSize(m_mode == ViewMode::Grid ? 11 : 12);
        nf.setBold(true);
        nameLbl->setFont(nf);
        nameLbl->setTextColor(Mocha::Text);
        nameLbl->setText(m_entry.name);

        // ---- Exec label (Custom Elided) ----
        auto *execLbl = new ElidedLabel(this);
        QFont ef;
        ef.setPointSize(m_mode == ViewMode::Grid ? 8 : 9);
        execLbl->setFont(ef);
        execLbl->setTextColor(Mocha::Subtext);
        execLbl->setText(m_entry.exec);

        // ---- Three-dot options button & Menu logic ----
        auto *optBtn = new OptionsButton(this);
        connect(optBtn, &QPushButton::clicked, this, [this, optBtn]() {
            QMenu menu(this);
            menu.setCursor(Qt::PointingHandCursor);

            QAction *actCopyName = menu.addAction("Copy Application Name");
            QAction *actCopyExec = menu.addAction("Copy Execution Command");
            QAction *actCopyPath = menu.addAction("Copy .desktop File Path");
            menu.addSeparator();
            QAction *actOpenEditor = menu.addAction("Open .desktop in Editor");
            QAction *actLaunch = menu.addAction("Launch Application");

            connect(actCopyName, &QAction::triggered, [this](){
                QGuiApplication::clipboard()->setText(m_entry.name);
            });
            connect(actCopyExec, &QAction::triggered, [this](){
                QGuiApplication::clipboard()->setText(m_entry.exec);
            });
            connect(actCopyPath, &QAction::triggered, [this](){
                QGuiApplication::clipboard()->setText(m_entry.filePath);
            });
            connect(actOpenEditor, &QAction::triggered, [this](){
                QDesktopServices::openUrl(QUrl::fromLocalFile(m_entry.filePath));
            });
            connect(actLaunch, &QAction::triggered, [this](){
                QStringList args = QProcess::splitCommand(m_entry.exec);
                if (!args.isEmpty()) {
                    QString prog = args.takeFirst();
                    QProcess::startDetached(prog, args);
                }
            });

            // Show menu under the button aligned to the right
            QPoint pos = optBtn->mapToGlobal(QPoint(optBtn->width() - menu.sizeHint().width(), optBtn->height()));
            menu.exec(pos);
        });

        // ============ GRID MODE ============
        if (m_mode == ViewMode::Grid) {
            setFixedHeight(176);
            setMinimumWidth(190);
            setMaximumWidth(16777215);
            setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

            auto *lay = new QVBoxLayout(this);
            lay->setAlignment(Qt::AlignCenter);
            lay->setSpacing(6);
            lay->setContentsMargins(14, 18, 14, 14);

            auto *topRow = new QHBoxLayout;
            topRow->addStretch();
            topRow->addWidget(optBtn);
            lay->addLayout(topRow);

            lay->addWidget(iconLbl, 0, Qt::AlignCenter);

            nameLbl->setAlignment(Qt::AlignCenter);
            lay->addWidget(nameLbl, 0);

            execLbl->setAlignment(Qt::AlignCenter);
            lay->addWidget(execLbl, 0);
        }
        // ============ LIST MODE ============
        else {
            setFixedHeight(68);
            setMinimumWidth(0);
            setMaximumWidth(16777215);
            setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

            auto *row = new QHBoxLayout(this);
            row->setContentsMargins(16, 8, 12, 8);
            row->setSpacing(14);

            row->addWidget(iconLbl, 0, Qt::AlignVCenter);

            auto *textCol = new QVBoxLayout;
            textCol->setSpacing(2);
            textCol->setContentsMargins(0, 0, 0, 0);

            nameLbl->setAlignment(Qt::AlignLeft);
            nameLbl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
            textCol->addWidget(nameLbl);

            execLbl->setAlignment(Qt::AlignLeft);
            execLbl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
            textCol->addWidget(execLbl);

            // By setting Stretch to 1, the text column forces the 3-dots button to the extreme right
            row->addLayout(textCol, 1);

            row->addWidget(optBtn, 0, Qt::AlignVCenter | Qt::AlignRight);
        }
    }
};

// =============================================================================
//  GlowWindow  —  custom window with mouse-following radial gradient
// =============================================================================
class GlowWindow : public QWidget {
    Q_OBJECT

public:
    explicit GlowWindow(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setWindowTitle(QStringLiteral("Desktop File Viewer"));
        resize(1120, 760);
        setMouseTracking(true);

        qApp->installEventFilter(this);

        m_timer = new QTimer(this);
        connect(m_timer, &QTimer::timeout, this, &GlowWindow::interpolateGlow);
        m_timer->start(16);
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        p.fillRect(rect(), Mocha::Base);

        QRadialGradient grad(m_glowPos, 420);
        grad.setColorAt(0.0, Mocha::Glow());
        grad.setColorAt(1.0, Qt::transparent);
        p.fillRect(rect(), grad);
    }

    bool eventFilter(QObject *obj, QEvent *ev) override {
        if (ev->type() == QEvent::MouseMove) {
            auto *w = qobject_cast<QWidget *>(obj);
            if (w && (w == this || isAncestorOf(w))) {
                auto *me = static_cast<QMouseEvent *>(ev);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                QPoint local = me->position().toPoint();
#else
                QPoint local = me->pos();
#endif
                m_mouseTarget = w->mapTo(this, local);
            }
        }
        return QWidget::eventFilter(obj, ev);
    }

private slots:
    void interpolateGlow() {
        constexpr qreal kSmooth = 0.07;
        m_glowPos += (QPointF(m_mouseTarget) - m_glowPos) * kSmooth;
        update();
    }

private:
    QPoint  m_mouseTarget{400, 300};
    QPointF m_glowPos    {400, 300};
    QTimer *m_timer = nullptr;
};

// =============================================================================
//  Icon helper
// =============================================================================
static QIcon loadIcon(const QString &name) {
    if (name.isEmpty())
        return QIcon::fromTheme(QStringLiteral("application-x-executable"));

    if (QFile::exists(name))
        return QIcon(name);

    QIcon themed = QIcon::fromTheme(name);
    if (!themed.isNull()) return themed;

    const QString home = QDir::homePath();
    const QStringList dirs = {
        QStringLiteral("/usr/share/pixmaps/"),
        QStringLiteral("/usr/share/icons/hicolor/48x48/apps/"),
        QStringLiteral("/usr/share/icons/hicolor/64x64/apps/"),
        QStringLiteral("/usr/share/icons/hicolor/128x128/apps/"),
        QStringLiteral("/usr/share/icons/hicolor/256x256/apps/"),
        QStringLiteral("/usr/share/icons/hicolor/scalable/apps/"),
        home + QStringLiteral("/.local/share/icons/hicolor/48x48/apps/"),
        home + QStringLiteral("/.local/share/icons/hicolor/64x64/apps/"),
        home + QStringLiteral("/.local/share/icons/hicolor/128x128/apps/"),
        home + QStringLiteral("/.local/share/icons/hicolor/256x256/apps/"),
        home + QStringLiteral("/.local/share/icons/hicolor/scalable/apps/"),
        home + QStringLiteral("/.icons/hicolor/48x48/apps/"),
        home + QStringLiteral("/.icons/hicolor/64x64/apps/"),
        home + QStringLiteral("/.icons/hicolor/128x128/apps/"),
        home + QStringLiteral("/.icons/hicolor/256x256/apps/"),
        home + QStringLiteral("/.icons/hicolor/scalable/apps/"),
        home + QStringLiteral("/.local/share/icons/"),
        home + QStringLiteral("/.icons/"),
    };
    static const QStringList exts = {
        QStringLiteral(".png"),
        QStringLiteral(".svg"),
        QStringLiteral(".xpm"),
        QStringLiteral(".ico"),
    };
    for (const auto &d : dirs)
        for (const auto &x : exts)
            if (QFile::exists(d + name + x))
                return QIcon(d + name + x);

    return QIcon::fromTheme(QStringLiteral("application-x-executable"));
}


static QVector<DesktopEntry> parseDesktopFiles() {
    const QStringList searchDirs = {
        QStringLiteral("/usr/share/applications"),
        QDir::homePath() + QStringLiteral("/.local/share/applications"),
        QStringLiteral("/var/lib/flatpak/exports/share/applications"),
        QDir::homePath() + QStringLiteral("/.local/share/flatpak/exports/share/applications"),
        QStringLiteral("/var/lib/snapd/desktop/applications"),
    };

    QVector<DesktopEntry> out;
    QSet<QString> seen;

    for (const auto &root : searchDirs) {
        if (!QDir(root).exists()) continue;

        QDirIterator it(root, {QStringLiteral("*.desktop")},
                        QDir::Files, QDirIterator::Subdirectories);

        while (it.hasNext()) {
            it.next();
            const QString baseName = it.fileName();
            if (seen.contains(baseName)) continue;

            QFile f(it.filePath());
            if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) continue;

            DesktopEntry entry;
            entry.filePath = it.filePath();
            bool inSection = false;
            bool noDisplay = false;
            bool hidden    = false;

            QTextStream ts(&f);
            while (!ts.atEnd()) {
                const QString line = ts.readLine().trimmed();

                if (line.startsWith(QLatin1Char('['))) {
                    inSection = (line == QStringLiteral("[Desktop Entry]"));
                    continue;
                }
                if (!inSection) continue;

                if      (line.startsWith(QStringLiteral("Name=")) && entry.name.isEmpty())
                    entry.name = line.mid(5);
                else if (line.startsWith(QStringLiteral("GenericName=")) && entry.genericName.isEmpty())
                    entry.genericName = line.mid(12);
                else if (line.startsWith(QStringLiteral("Icon=")) && entry.iconName.isEmpty())
                    entry.iconName = line.mid(5);
                else if (line.startsWith(QStringLiteral("Exec=")) && entry.exec.isEmpty()) {
                    entry.exec = line.mid(5);
                    entry.exec.remove(QRegularExpression(QStringLiteral("%[fFuUdDnNickvm]")));
                    entry.exec = entry.exec.trimmed();
                }
                else if (line.startsWith(QStringLiteral("Keywords=")) && entry.keywords.isEmpty())
                    entry.keywords = line.mid(9);
                else if (line == QStringLiteral("NoDisplay=true"))
                    noDisplay = true;
                else if (line == QStringLiteral("Hidden=true"))
                    hidden = true;
            }

            if (!entry.name.isEmpty() && !noDisplay && !hidden) {
                seen.insert(baseName);
                out.append(entry);
            }
        }
    }

    std::sort(out.begin(), out.end(), [](const DesktopEntry &a, const DesktopEntry &b) {
        return a.name.toLower() < b.name.toLower();
    });
    return out;
}


static QString globalStylesheet() {
    return QStringLiteral(R"QSS(

    * {
        font-family: 'Inter', 'Segoe UI', 'Roboto', 'Noto Sans', sans-serif;
    }

    QScrollArea, QScrollArea > QWidget > QWidget {
        background: transparent;
        border: none;
    }

    QScrollBar:vertical {
        background: transparent;
        width: 8px;
        margin: 4px 2px;
    }
    QScrollBar::handle:vertical {
        background: #45475a;
        border-radius: 4px;
        min-height: 32px;
    }
    QScrollBar::handle:vertical:hover { background: #585b70; }
    QScrollBar::add-line:vertical,
    QScrollBar::sub-line:vertical,
    QScrollBar::add-page:vertical,
    QScrollBar::sub-page:vertical {
        background: none; border: none; height: 0px;
    }

    QScrollBar:horizontal {
        background: transparent;
        height: 8px;
        margin: 2px 4px;
    }
    QScrollBar::handle:horizontal {
        background: #45475a;
        border-radius: 4px;
        min-width: 32px;
    }
    QScrollBar::handle:horizontal:hover { background: #585b70; }
    QScrollBar::add-line:horizontal,
    QScrollBar::sub-line:horizontal,
    QScrollBar::add-page:horizontal,
    QScrollBar::sub-page:horizontal {
        background: none; border: none; width: 0px;
    }

    QLineEdit {
        background: #313244;
        color: #cdd6f4;
        border: 2px solid #45475a;
        border-radius: 12px;
        padding: 10px 16px;
        font-size: 14px;
        selection-background-color: #cba6f7;
        selection-color: #1e1e2e;
    }
    QLineEdit:focus {
        border-color: #cba6f7;
    }
    QLineEdit::placeholder {
        color: #6c7086;
    }

    QToolTip {
        background: #313244;
        color: #cdd6f4;
        border: 1px solid #45475a;
        border-radius: 6px;
        padding: 6px 10px;
        font-size: 12px;
    }
    
    /* ---- QMenu Styling (Mocha) ---- */
    QMenu {
        background-color: #313244;
        color: #cdd6f4;
        border: 1px solid #45475a;
        border-radius: 8px;
        padding: 4px;
        font-size: 13px;
    }
    QMenu::item {
        padding: 8px 24px;
        border-radius: 4px;
        margin: 2px 4px;
    }
    QMenu::item:selected {
        background-color: #45475a;
        color: #cba6f7;
    }
    QMenu::separator {
        height: 1px;
        background: #45475a;
        margin: 4px 8px;
    }

    )QSS");
}

static QString rippleButtonQSS() {
    return QStringLiteral(
        "RippleButton {"
        "  background: #313244;"
        "  color: #cdd6f4;"
        "  border: 2px solid #45475a;"
        "  border-radius: 12px;"
        "  padding: 10px 22px;"
        "  font-size: 14px;"
        "  font-weight: bold;"
        "}"
        "RippleButton:hover {"
        "  background: #45475a;"
        "  border-color: #cba6f7;"
        "}"
        "RippleButton:pressed {"
        "  background: #585b70;"
        "}"
    );
}



static constexpr int kGridCols = 4;

static void createCards(
    QVector<AppCard *>            &cards,
    const QVector<DesktopEntry>   &entries,
    ViewMode                       mode,
    QWidget                       *parent)
{
    for (auto *c : cards) c->deleteLater();
    cards.clear();
    cards.reserve(entries.size());

    for (const auto &e : entries) {
        auto *card = new AppCard(e, loadIcon(e.iconName), mode, parent);
        card->setVisible(false);
        cards.append(card);
    }
}

static int reflowLayout(
    QVector<AppCard *>  &cards,
    QGridLayout         *grid,
    ViewMode             mode,
    const QString       &query)
{
    for (auto *c : cards)
        grid->removeWidget(c);

    for (int r = 0; r < grid->rowCount(); ++r)
        grid->setRowStretch(r, 0);
    for (int c = 0; c < kGridCols; ++c)
        grid->setColumnStretch(c, 0);

    if (mode == ViewMode::Grid) {
        for (int c = 0; c < kGridCols; ++c)
            grid->setColumnStretch(c, 1);
    } else {
        grid->setColumnStretch(0, 1);
    }

    int visibleCount = 0;
    for (auto *card : cards) {
        if (card->entry().matches(query)) {
            card->setVisible(true);
            if (mode == ViewMode::Grid)
                grid->addWidget(card, visibleCount / kGridCols,
                                      visibleCount % kGridCols);
            else
                grid->addWidget(card, visibleCount, 0);
            ++visibleCount;
        } else {
            card->setVisible(false);
        }
    }

    grid->setRowStretch(grid->rowCount(), 1);

    return visibleCount;
}

// =============================================================================
//  main
// =============================================================================
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setStyleSheet(globalStylesheet());

    QVector<DesktopEntry> entries = parseDesktopFiles();
    ViewMode currentMode = ViewMode::Grid;

    GlowWindow window;
    window.setMinimumSize(800, 580);

    auto *root = new QVBoxLayout(&window);
    root->setContentsMargins(34, 28, 34, 28);
    root->setSpacing(18);

    auto *title = new QLabel(QStringLiteral("Desktop Applications"));
    {
        QFont f;
        f.setPointSize(26);
        f.setBold(true);
        title->setFont(f);
    }
    title->setStyleSheet(
        QStringLiteral("color:%1;background:transparent;").arg(Mocha::Text.name()));
    root->addWidget(title);

    auto *subtitle = new QLabel(
        QStringLiteral("%1 applications found").arg(entries.size()));
    {
        QFont f;
        f.setPointSize(11);
        subtitle->setFont(f);
    }
    subtitle->setStyleSheet(
        QStringLiteral("color:%1;background:transparent;").arg(Mocha::Subtext.name()));
    root->addWidget(subtitle);

    root->addSpacing(2);

    auto *searchRow = new QHBoxLayout;
    searchRow->setSpacing(10);

    auto *searchBar = new QLineEdit;
    searchBar->setPlaceholderText(QStringLiteral("  Search by name, keyword, command\u2026"));
    searchBar->setMinimumHeight(44);
    searchRow->addWidget(searchBar, 1);

    auto *viewToggleBtn = new RippleButton(QStringLiteral("\u25A6  Grid"));
    viewToggleBtn->setMinimumHeight(44);
    viewToggleBtn->setMinimumWidth(110);
    viewToggleBtn->setStyleSheet(rippleButtonQSS());
    viewToggleBtn->setToolTip(QStringLiteral("Toggle between Grid and List view"));
    searchRow->addWidget(viewToggleBtn);

    auto *refreshBtn = new RippleButton(QStringLiteral("\u21BB  Refresh"));
    refreshBtn->setMinimumHeight(44);
    refreshBtn->setMinimumWidth(128);
    refreshBtn->setStyleSheet(rippleButtonQSS());
    searchRow->addWidget(refreshBtn);

    root->addLayout(searchRow);

    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setFrameShape(QFrame::NoFrame);

    auto *container = new QWidget;
    container->setStyleSheet(QStringLiteral("background:transparent;"));

    auto *grid = new QGridLayout(container);
    grid->setSpacing(14);
    grid->setContentsMargins(2, 2, 2, 2);

    QVector<AppCard *> cards;
    createCards(cards, entries, currentMode, container);

    scroll->setWidget(container);
    scroll->viewport()->setStyleSheet(QStringLiteral("background:transparent;"));
    root->addWidget(scroll, 1);

    auto updateLayout = [&cards, grid, subtitle, &currentMode](const QString &query) {
        int vis = reflowLayout(cards, grid, currentMode, query);
        subtitle->setText(QStringLiteral("%1 applications found").arg(vis));
    };

    updateLayout(QString());

    QObject::connect(searchBar, &QLineEdit::textChanged,
        [updateLayout](const QString &query) {
            updateLayout(query);
        });

    QObject::connect(viewToggleBtn, &QPushButton::clicked,
        [&currentMode, &cards, viewToggleBtn, searchBar, updateLayout]() {
            currentMode = (currentMode == ViewMode::Grid) ? ViewMode::List : ViewMode::Grid;
            viewToggleBtn->setText(
                currentMode == ViewMode::Grid
                    ? QStringLiteral("\u25A6  Grid")
                    : QStringLiteral("\u2630  List"));

            for (auto *c : cards)
                c->setViewMode(currentMode);

            updateLayout(searchBar->text());
        });

    QObject::connect(refreshBtn, &QPushButton::clicked,
        [&entries, &cards, grid, searchBar, &currentMode, container, updateLayout]() {
            entries = parseDesktopFiles();
            createCards(cards, entries, currentMode, container);
            updateLayout(QString());
            searchBar->clear();
        });

    window.show();
    return app.exec();
}

#include "main.moc"