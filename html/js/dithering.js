const bwryPalette = [
  [0, 0, 0, 255],       // Black
  [255, 255, 255, 255], // White
  [255, 0, 0, 255],     // Red
  [255, 255, 0, 255]    // Yellow
];

const bwrPalette = [
  [0, 0, 0, 255],
  [255, 255, 255, 255],
  [255, 0, 0, 255]
]

const bwPalette = [
  [0, 0, 0, 255],
  [255, 255, 255, 255],
]

function dithering(ctx, width, height, threshold, type) {
  const bayerThresholdMap = [
    [  15, 135,  45, 165 ],
    [ 195,  75, 225, 105 ],
    [  60, 180,  30, 150 ],
    [ 240, 120, 210,  90 ]
  ];

  const lumR = [];
  const lumG = [];
  const lumB = [];
  for (let i=0; i<256; i++) {
    lumR[i] = i*0.299;
    lumG[i] = i*0.587;
    lumB[i] = i*0.114;
  }
  const imageData = ctx.getImageData(0, 0, width, height);

  const imageDataLength = imageData.data.length;

  // Greyscale luminance (sets r pixels to luminance of rgb)
  for (let i = 0; i <= imageDataLength; i += 4) {
    imageData.data[i] = Math.floor(lumR[imageData.data[i]] + lumG[imageData.data[i+1]] + lumB[imageData.data[i+2]]);
  }

  const w = imageData.width;
  let newPixel, err;

  for (let currentPixel = 0; currentPixel <= imageDataLength; currentPixel+=4) {
    if (type === "gray") {
      const factor = 255 / (threshold - 1);
      imageData.data[currentPixel] = Math.round(imageData.data[currentPixel] / factor) * factor;
    } else if (type ==="none") {
      // No dithering
      imageData.data[currentPixel] = imageData.data[currentPixel] < threshold ? 0 : 255;
    } else if (type ==="bayer") {
      // 4x4 Bayer ordered dithering algorithm
      var x = currentPixel/4 % w;
      var y = Math.floor(currentPixel/4 / w);
      var map = Math.floor( (imageData.data[currentPixel] + bayerThresholdMap[x%4][y%4]) / 2 );
      imageData.data[currentPixel] = (map < threshold) ? 0 : 255;
    } else if (type ==="floydsteinberg") {
      // Floyda€"Steinberg dithering algorithm
      newPixel = imageData.data[currentPixel] < 129 ? 0 : 255;
      err = Math.floor((imageData.data[currentPixel] - newPixel) / 16);
      imageData.data[currentPixel] = newPixel;

      imageData.data[currentPixel       + 4 ] += err*7;
      imageData.data[currentPixel + 4*w - 4 ] += err*3;
      imageData.data[currentPixel + 4*w     ] += err*5;
      imageData.data[currentPixel + 4*w + 4 ] += err*1;
    } else {
      // Bill Atkinson's dithering algorithm
      newPixel = imageData.data[currentPixel] < threshold ? 0 : 255;
      err = Math.floor((imageData.data[currentPixel] - newPixel) / 8);
      imageData.data[currentPixel] = newPixel;

      imageData.data[currentPixel       + 4 ] += err;
      imageData.data[currentPixel       + 8 ] += err;
      imageData.data[currentPixel + 4*w - 4 ] += err;
      imageData.data[currentPixel + 4*w     ] += err;
      imageData.data[currentPixel + 4*w + 4 ] += err;
      imageData.data[currentPixel + 8*w     ] += err;
    }

    // Set g and b pixels equal to r
    imageData.data[currentPixel + 1] = imageData.data[currentPixel + 2] = imageData.data[currentPixel];
  }

  ctx.putImageData(imageData, 0, 0);
}

// white: 1, black/red: 0
function canvas2bytes(canvas, step = 'bw', invert = false) {
  const ctx = canvas.getContext("2d");
  const imageData = ctx.getImageData(0, 0, canvas.width, canvas.height);

  const arr = [];
  let buffer = [];

  for (let y = 0; y < canvas.height; y++) {
    for (let x = 0; x < canvas.width; x++) {
      const i = (canvas.width * y + x) * 4;
      const r = imageData.data[i];
      const g = imageData.data[i + 1];
      const b = imageData.data[i + 2];

      if (step === 'bwry') {
        // 四色模式：黑(00)、白(01)、黄(10)、红(11)
        let pixelBits;
        if (r < 50 && g < 50 && b < 50) {
          pixelBits = 0b00; // 黑色
        } else if (r > 200 && g > 200 && b > 200) {
          pixelBits = 0b01; // 白色
        } else if (r > 200 && g > 200 && b < 50) {
          pixelBits = 0b10; // 黄色
        } else if (r > 200 && g < 50 && b < 50) {
          pixelBits = 0b11; // 红色
        } else {
          pixelBits = 0b01; // 默认白色
        }
        buffer.push(pixelBits);

        // 每4个像素（8 bits）合并为1字节
        if (buffer.length === 4) {
          const byte = (buffer[0] << 6) | (buffer[1] << 4) | (buffer[2] << 2) | buffer[3];
          arr.push(invert ? ~byte & 0xFF : byte);
          buffer = [];
        }
      } else {
        // 黑白或红色模式
        let pixelValue;
        if (step === 'bw') {
          pixelValue = (r === 0 && g === 0 && b === 0) ? 0 : 1;
        } else {
          pixelValue = (r > 0 && g === 0 && b === 0) ? 0 : 1;
        }
        buffer.push(pixelValue);

        if (buffer.length === 8) {
          const byte = parseInt(buffer.join(''), 2);
          arr.push(invert ? ~byte : byte);
          buffer = [];
        }
      }
    }
  }

  if (buffer.length > 0) {
    if (step === 'bwry') {
      // 四色模式：填充白色(01)至4个像素
      while (buffer.length < 4) {
        buffer.push(0b01);
      }
      const byte = (buffer[0] << 6) | (buffer[1] << 4) | (buffer[2] << 2) | buffer[3];
      arr.push(invert ? ~byte & 0xFF : byte);
    } else {
      while (buffer.length < 8) {
        buffer.push(1);
      }
      const byte = parseInt(buffer.join(''), 2);
      arr.push(invert ? ~byte : byte);
    }
  }
  return arr;
}

function getColorDistance(rgba1, rgba2) {
  const [r1, b1, g1] = rgba1;
  const [r2, b2, g2] = rgba2;

  const rm = (r1 + r2 ) / 2;

  const r = r1 - r2;
  const g = g1 - g2;
  const b = b1 - b2;

  return Math.sqrt((2 + rm / 256) * r * r + 4 * g * g + (2 + (255 - rm) / 256) * b * b);
}

function getNearColor(pixel, palette) {
  let minDistance = 255 * 255 * 3 + 1;
  let paletteIndex = 0;

  for (let i = 0; i < palette.length; i++) {
    const targetColor = palette[i];
    const distance = getColorDistance(pixel, targetColor);
    if (distance < minDistance) {
      minDistance = distance;
      paletteIndex = i;
    }
  }

  return palette[paletteIndex];
}


function getNearColorV2(color, palette) {
  let minDistanceSquared = 255*255 + 255*255 + 255*255 + 1;

  let bestIndex = 0;
  for (let i = 0; i < palette.length; i++) {
      let rdiff = (color[0] & 0xff) - (palette[i][0] & 0xff);
      let gdiff = (color[1] & 0xff) - (palette[i][1] & 0xff);
      let bdiff = (color[2] & 0xff) - (palette[i][2] & 0xff);
      let distanceSquared = rdiff*rdiff + gdiff*gdiff + bdiff*bdiff;
      if (distanceSquared < minDistanceSquared) {
          minDistanceSquared = distanceSquared;
          bestIndex = i;
      }
  }
  return palette[bestIndex];

}


function updatePixel(imageData, index, color) {
  imageData[index] = color[0];
  imageData[index+1] = color[1];
  imageData[index+2] = color[2];
  imageData[index+3] = color[3];
}

function getColorErr(color1, color2, rate) {
  const res = [];
  for (let i = 0; i < 3; i++) {
    res.push(Math.floor((color1[i] - color2[i]) / rate));
  }
  return res;
}

function updatePixelErr(imageData, index, err, rate) {
  imageData[index] += err[0] * rate;
  imageData[index+1] += err[1] * rate;
  imageData[index+2] += err[2] * rate;
}

function ditheringCanvasByPalette(canvas, palette, type) {
  palette = palette || bwrPalette;

  const ctx = canvas.getContext('2d');
  const imageData = ctx.getImageData(0, 0, canvas.width, canvas.height);
  const w = imageData.width;

  for (let currentPixel = 0; currentPixel <= imageData.data.length; currentPixel+=4) {
    const newColor = getNearColorV2(imageData.data.slice(currentPixel, currentPixel+4), palette);

    if (type === "bwr_floydsteinberg") {
      const err = getColorErr(imageData.data.slice(currentPixel, currentPixel+4), newColor, 16);

      updatePixel(imageData.data, currentPixel, newColor);
      updatePixelErr(imageData.data, currentPixel +4, err, 7);
      updatePixelErr(imageData.data, currentPixel + 4*w - 4, err, 3);
      updatePixelErr(imageData.data, currentPixel + 4*w, err, 5);
      updatePixelErr(imageData.data, currentPixel + 4*w + 4, err, 1);
    } else {
      const err = getColorErr(imageData.data.slice(currentPixel, currentPixel+4), newColor, 8);

      updatePixel(imageData.data, currentPixel, newColor);
      updatePixelErr(imageData.data, currentPixel +4, err, 1);
      updatePixelErr(imageData.data, currentPixel +8, err, 1);
      updatePixelErr(imageData.data, currentPixel +4 * w - 4, err, 1);
      updatePixelErr(imageData.data, currentPixel +4 * w, err, 1);
      updatePixelErr(imageData.data, currentPixel +4 * w + 4, err, 1);
      updatePixelErr(imageData.data, currentPixel +8 * w, err, 1);
    }
  }
  ctx.putImageData(imageData, 0, 0);
}