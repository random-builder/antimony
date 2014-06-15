#ifndef SHAPE_DATUM_H
#define SHAPE_DATUM_H

#include <Python.h>
#include "datum/datum.h"
#include "cpp/fab.h"

class MathShapeDatum : public Datum
{
    Q_OBJECT
public:
    explicit MathShapeDatum(QString name, QObject* parent=0);

    /** Overload function to get the current value; always fails
     *  (because the shape will always from the input handler)
     */
    virtual PyObject* getCurrentValue() { return NULL; }

    /** Returns the python MathShape class.
     */
    virtual PyTypeObject* getType() const { return fab::MathShapeType; }
};

#endif // SHAPE_DATUM_H
