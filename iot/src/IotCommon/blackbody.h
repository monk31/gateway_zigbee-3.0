// -----------------------------------------------------------------
// Black body curve - include file
// -----------------------------------------------------------------

typedef struct {
    int   kelvin;
    float x;
    float y;
    int   r;
    int   g;
    int   b;
} bb_point;

extern bb_point bb_curve[91];

