#ifndef NAMETABLEVIEW_HPP
#define NAMETABLEVIEW_HPP

#include <QWidget>
#include <QTimer>
#include <QPainter>
#include <iostream>
#include "ui_nametableview.h"
#include "NES.h"
#include "functions.hpp"

namespace Ui {
class NameTableView;
}

class NameTableView : public QWidget {
    Q_OBJECT
public:
    inline NameTableView(std::shared_ptr<NES> nes, QWidget *parent = nullptr);
    inline NameTableView(std::shared_ptr<NES> nes, bool shouldStepNes, QWidget *parent = nullptr);
    inline ~NameTableView() override;

private:
    inline void stepTimeTick();
    inline void noStepTimeTick();
    inline void paintEvent(QPaintEvent*) override;
    inline void paint();
    inline QColor getPalQColor(const uint8_t& colorByte) const;
    inline QColor getColor(const uint8_t& n) const;
    inline void setColorSet(const uint16_t&);
    Ppu::ColorSetT colorSet;
    Ui::NameTableView *ui;
    std::shared_ptr<NES> nes;
    QTimer* timer;
};

NameTableView::NameTableView(std::shared_ptr<NES> nes, QWidget *parent) : QWidget(parent), ui(new Ui::NameTableView) {
    ui->setupUi(this);
    this->nes = nes;
}

NameTableView::NameTableView(std::shared_ptr<NES> nes, bool shouldStepNes, QWidget *parent) : QWidget(parent), ui(new Ui::NameTableView) {
    ui->setupUi(this);
    this->nes = nes;
    timer = new QTimer(this);
    if (shouldStepNes) {
        connect(timer, &QTimer::timeout, this, &NameTableView::stepTimeTick);
        timer->start(0);
    }
    else {
        connect(timer, &QTimer::timeout, this, &NameTableView::noStepTimeTick);
        timer->start(3000);
    }
}

NameTableView::~NameTableView() {
    delete ui;
}

void NameTableView::stepTimeTick() {
    for(unsigned i = 0 ; i != 15; ++i) nes->step();

    if (nes->ppu.completeFrame) {
        nes->ppu.completeFrame = false;
        repaint();
    }
}

void NameTableView::noStepTimeTick() {
    // very hacky way to tell if the NES is running something
    if (nes->getBaseName().size() > 1)
        this->repaint();
}

void NameTableView::paintEvent(QPaintEvent *) {
    if (nes->getBaseName().size() > 1)
        paint();
}

QColor NameTableView::getPalQColor(const uint8_t& colorByte) const {
    Ppu::PaletteT universalPalette = Ppu::getRGBPalette(colorByte & 0x3F);
    return apply_from_tuple(qRgb, universalPalette);
}

QColor NameTableView::getColor(const uint8_t &n) const {
    switch(n) {
        case 0: return getPalQColor(std::get<0>(colorSet));
        case 1: return getPalQColor(std::get<1>(colorSet));
        case 2: return getPalQColor(std::get<2>(colorSet));
        case 3: return getPalQColor(std::get<3>(colorSet));
    default: std::cerr << "getColor(), no color"; return Qt::black;
    }
}

void NameTableView::setColorSet(const uint16_t& relNameTableAdr) {
    // PaletteId of the nametable determines which palette to talk to
    // and what bits the colours represent.
    uint8_t paletteID = nes->ppu.getPaletteFromNameTable(relNameTableAdr, 0x23C0);
    switch (paletteID) {
        case 0:
            colorSet = nes->ppu.getColorSetFromAdr(0x3F01); break;
        case 1:
            colorSet = nes->ppu.getColorSetFromAdr(0x3F05); break;
        case 2:
            colorSet = nes->ppu.getColorSetFromAdr(0x3F09); break;
        case 3:
            colorSet = nes->ppu.getColorSetFromAdr(0x3F0D); break;
        default:
            throw std::runtime_error("Could not get proper palette id");
    }
}

void NameTableView::paint() {
    QPainter painter(this);
    auto setColor = [&painter](const auto& color) {
        painter.setPen(color);
        painter.setBrush(color);
    };


    // Nametables : 0x2000, 0x2400, 0x2800, 0x2C00
    // Later can have it so the user can select it
    // After 0x23C0 is the attribute table
    uint16_t nameTableStart = 0x2000;

    for (uint16_t address = nameTableStart; address != nameTableStart + 0x3C0; address++) {
        uint16_t tileNum = address - nameTableStart;

        setColorSet(tileNum);
        // Address X and Y determine where to put the tile
        // The screen is a 32x30 tile screen, so to put it on, 32*8 and 30*8 for 256x240 NES screen size
        uint8_t addressX = tileNum % 32;
        uint8_t addressY = static_cast<uint8_t>(static_cast<int>(tileNum / 32));
        uint8_t byte = nes->ppu.vRamRead(address);
        // NOTE/TODO: It was noticed in donkey kong its always the 2nd/left pattern table
        // In other games it's probably determined by the bit in the ppu's ctrl
        Ppu::PatternTableT tile = nes->ppu.getPatternTile(byte, false);
        for (uint8_t tileY = 0; tileY != 8; tileY++) {
            uint16_t line = tile[tileY];
            for (uint8_t tileX = 0; tileX != 8; tileX++) {
                uint8_t pixel = ( line >> (tileX * 2) ) & 0b11; // two bits determine the color of bit (0, 1, 2)
                setColor(getColor(pixel));

                int pixelX = addressX * 8 + tileX;
                int pixelY = addressY * 8 + tileY;


                painter.drawRect(pixelX, pixelY, 1, 1);
            }
        }
    }

}


#endif // NAMETABLEVIEW_HPP
