#include <Python.h>

#include "render/render_image.h"
#include "ui/canvas.h"
#include "ui/depth_image.h"

#include "cpp/shape.h"
#include "util/region.h"
#include "tree/render.h"

#include "formats/png.h"

RenderImage::RenderImage(Bounds b, QVector3D pos, float scale)
    : QObject(), bounds(b), pos(pos), scale(scale),
      depth((b.xmax - b.xmin) * scale,
            (b.ymax - b.ymin) * scale,
            QImage::Format_RGB32),
      shaded(depth.width(), depth.height(), depth.format())
{
    // Nothing to do here
    // (render() must be called explicity)
}

RenderImage::~RenderImage()
{
    for (auto p = pixmaps.begin(); p != pixmaps.end(); ++p)
    {
        p.value()->deleteLater();
    }
}

void RenderImage::render(Shape *shape)
{
    depth.fill(0x000000);

    uint8_t* d8(new uint8_t[depth.width() * depth.height()]);
    uint8_t** d8_rows(new uint8_t*[depth.height()]);
    uint8_t (*s8)[3] = new uint8_t[depth.width() * depth.height()][3];
    uint8_t (**s8_rows)[3] = new decltype(s8)[depth.height()];

    for (int i=0; i < depth.height(); ++i)
    {
        d8_rows[i] = d8 + (depth.width() * i);
        s8_rows[i] = s8 + (depth.width() * i);
    }
    memset(d8, 0, depth.width() * depth.height());
    memset(s8, 0, depth.width() * depth.height() * 3);

    Region r = (Region) {
            .imin=0, .jmin=0, .kmin=0,
            .ni=(uint32_t)depth.width(), .nj=(uint32_t)depth.height(),
            .nk=uint32_t(fmax(1, (shape->bounds.zmax -
                                  shape->bounds.zmin) * scale))
    };

    build_arrays(&r, shape->bounds.xmin, shape->bounds.ymin, shape->bounds.zmin,
                     shape->bounds.xmax, shape->bounds.ymax, shape->bounds.zmax);
    int halt=0;
    render8(shape->tree.get(), r, d8_rows, &halt);
    shaded8(shape->tree.get(), r, d8_rows, s8_rows, &halt);

    free_arrays(&r);

    for (int j=0; j < depth.height(); ++j)
    {
        for (int i=0; i < depth.width(); ++i)
        {
            uint8_t pix = d8_rows[j][i];
            uint8_t* norm = s8_rows[j][i];
            if (pix)
            {
                depth.setPixel(i, depth.height() - j - 1,
                               pix | (pix << 8) | (pix << 16));
                shaded.setPixel(i, depth.height() - j - 1,
                        norm[0] | (norm[1] << 8) | (norm[2] << 16));
            }
        }
    }

    delete [] s8;
    delete [] s8_rows;
    delete [] d8;
    delete [] d8_rows;

}

void RenderImage::applyGradient(bool direction)
{
    for (int j=0; j < depth.height(); ++j)
    {
        for (int i=0; i < depth.width(); ++i)
        {
            uint8_t pix = depth.pixel(i, j) & 0xff;
            if (pix)
            {
                if (direction)
                {
                    pix *= j / float(depth.height());
                }
                else
                {
                    pix *= 1 - j / float(depth.height());
                }
                depth.setPixel(i, j, pix | (pix << 8) | (pix << 16));
            }
        }
    }
}

void RenderImage::addToCanvas(Canvas *canvas)
{
    DepthImageItem* pix = new DepthImageItem(pos,
            QVector3D(bounds.xmax - bounds.xmin,
                      bounds.ymax - bounds.ymin,
                      bounds.zmax - bounds.zmin), depth, shaded, canvas);
    canvas->scene->addItem(pix);
    pixmaps[canvas] = pix;
}

