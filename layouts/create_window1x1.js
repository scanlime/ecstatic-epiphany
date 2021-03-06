/*
 * Early prototype, single square (1x1) with 10 LEDs per side.
 *
 * The JSON for this layout includes multiple kinds of data about
 * each LED. Each LED has the following fields:
 *
 *        point: A 3D vector, arbitrary coordinates. Scaled to look good
 *               in the Open Pixel Control "gl_server" pre-visualizer
 *
 *       gridXY: Integer xy location of the block in the overall window grid
 *      blockXY: Location within the block, in XY coordinates within [-1, 1]
 *   blockAngle: Angle within the block, in radians. Zero is +Y.
 *
 * 2014 Micah Elizabeth Scott
 * This file is released into the public domain.
 */

var model = []
var blockSize = 0.3;
var centerX = blockSize * 1/2;
var centerY = blockSize * 1/2;

function blockEdge(index, gridXY, angle)
{
    // Lay out one LED strip corresponding to a block edge

    var count = 10;        // How many LEDs?
    var y = 0.75;          // Distance from center, in model

    var spacing = 2 * y / (count + 1);
    var s = Math.sin(angle);
    var c = Math.cos(angle);

    for (var i = 0; i < count; i++) {
        // Distance from vertical Y axis
        var x = (i - (count-1)/2.0) * spacing;

        // Rotated XY
        var rx = x * c - y * s;
        var ry = x * s + y * c;

        model[index++] = {
            point: [
                blockSize * -(gridXY[0] + rx * 0.5 + 0.5) + centerX,
                0,
                blockSize * -(gridXY[1] - ry * 0.5 + 0.5) + centerY
            ],
            gridXY: gridXY,
            blockXY: [rx, ry],
            blockAngle: Math.atan2(rx, ry)
        }
    }
}

function block(index, gridXY)
{
    for (var i = 0; i < 4; i++) {
        blockEdge(index + i * 10, gridXY, i * -Math.PI / 2);
    }
}

block(0, [0, 0]);

console.log(JSON.stringify(model));
