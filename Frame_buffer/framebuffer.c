#include<unistd.h>
#include<stdio.h>
#include<fcntl.h>
#include<linux/fb.h>
#include<sys/mman.h>
#include "inc_camera.h"
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>

#define	height	1080
#define	width	1920	//12345

#define	height2	480
#define	width2	720

static unsigned char *input_y = NULL;
static unsigned char *input_u = NULL;
static unsigned char *input_v = NULL;

int *r = NULL, *g = NULL, *b = NULL;
int R[width2][height2];
int G[width2][height2];
int B[width2][height2];

int R2[width2][height2];
int G2[width2][height2];
int B2[width2][height2];

void YUVtoRGB(unsigned char *, unsigned char *, unsigned char *);
void toYUV(unsigned char *);
void Framebuf(unsigned char *);

int main(void)
{
	int dev = 0, frame = 1, i = 0;
	unsigned int pixelformat = V4L2_PIX_FMT_YUYV;
	unsigned int nbufs = V4L_BUFFERS_DEFAULT;
	unsigned int input = 0;
	/*-----------------------------------------------------------------------------------*/
	int fp = 0;
	struct fb_var_screeninfo vinfo;
	struct fb_fix_screeninfo finfo;
	long screensize = 0;
	char* fbp = 0;
	int x = 0, y = 0;
	long location = 0;
	/*-----------------------------------------------------------------------------------*/
	BT656_buffer = (unsigned char *)malloc(sizeof(unsigned char) * lsize*wsize * 2);

	dev = video_open("/dev/video0");
	camera_init(input, pixelformat, nbufs, 0, dev, 720, 480);
	memset(&buf, 0, sizeof buf);
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;

	/*-----------------------------------------------------------------------------------*/
	fp = open("/dev/fb0", O_RDWR);
	if (fp < 0)
	{
		printf("Open device failed.\n");
		exit(1);
	}
	if (ioctl(fp, FBIOGET_FSCREENINFO, &finfo))
	{
		printf("ERROR reading fixed info\n");
		exit(2);
	}
	printf("mem = %d\n", finfo.smem_len);
	printf("line_length = %d\n", finfo.line_length);


	if (ioctl(fp, FBIOGET_VSCREENINFO, &vinfo))
	{
		printf("ERROR reading variable info\n");
		exit(3);
	}
	printf("xres = %d\n", vinfo.xres);
	printf("yres = %d\n", vinfo.yres);
	printf("bits_per_pixel = %d\n", vinfo.bits_per_pixel);

	screensize = vinfo.xres*vinfo.yres*vinfo.bits_per_pixel / 8;
	fbp = (char*)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fp, 0);
	if ((int)fbp == -1)
	{
		printf("ERROR to map\n");
		exit(4);
	}
	/*-----------------------------------------------------------------------------------*/

	while (1)
	{
		ioctl(dev, VIDIOC_DQBUF, &buf);
		memcpy(BT656_buffer, mem[buf.index], 720 * 480 * 2);
		toYUV(BT656_buffer);
		Framebuf(BT656_buffer);
		ioctl(dev, VIDIOC_QBUF, &buf);
		printf("frame=%d\n", frame);
		frame++;
		fflush(stdout);
		i++;
	}
	return 0;
}

void toYUV(unsigned char *BT656_buffer)
{
	input_y = (unsigned char *)malloc(sizeof(unsigned char)*height2*width2);
	input_u = (unsigned char *)malloc(sizeof(unsigned char)*height2*width2 / 2);
	input_v = (unsigned char *)malloc(sizeof(unsigned char)*height2*width2 / 2);

	int a = 0, b = 0, c = 0, x;

	for (x = 0; x < lsize*wsize * 2; x++)
	{
		if (x % 2 == 0)
		{
			input_y[a] = BT656_buffer[x];
			a++;
		}
		else if (x % 2 == 1 && x % 4 == 1)
		{
			input_u[b] = BT656_buffer[x];
			b++;
		}
		else if (x % 4 == 3)
		{
			input_v[c] = BT656_buffer[x];
			c++;
		}
	}
}
void Framebuf(unsigned char *BT656_buffer)
{
	/*int fp = 0;
	struct fb_var_screeninfo vinfo;
	struct fb_fix_screeninfo finfo;
	long screensize = 0;
	char* fbp = 0;
	int x = 0, y = 0;
	long location = 0;

	fp = open("/dev/fb0", O_RDWR);
	if (fp < 0)
	{
		printf("Open device failed.\n");
		exit(1);
	}
	if (ioctl(fp, FBIOGET_FSCREENINFO, &finfo))
	{
		printf("ERROR reading fixed info\n");
		exit(2);
	}
	printf("mem = %d\n", finfo.smem_len);
	printf("line_length = %d\n", finfo.line_length);


	if (ioctl(fp, FBIOGET_VSCREENINFO, &vinfo))
	{
		printf("ERROR reading variable info\n");
		exit(3);
	}
	printf("xres = %d\n", vinfo.xres);
	printf("yres = %d\n", vinfo.yres);
	printf("bits_per_pixel = %d\n", vinfo.bits_per_pixel);

	screensize = vinfo.xres*vinfo.yres*vinfo.bits_per_pixel / 8;
	fbp = (char*)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fp, 0);
	if ((int)fbp == -1)
	{
		printf("ERROR to map\n");
		exit(4);
	}*/


	while (input_y)
	{
		for (y = 0; y < height; y++)
		{
			for (x = 0; x < width; x++)
			{
				location = x*(vinfo.bits_per_pixel / 8) + y*finfo.line_length;

				if (x < width2 && y < height2)
				{
					YUVtoRGB(input_y, input_u, input_v);

					*(fbp + location) = R2[x][y];	//R
					*(fbp + location + 1) = G2[x][y];  //G
					*(fbp + location + 2) = B2[x][y]; 	//B
					*(fbp + location + 3) = 0;
				}
			}
		}
	}
	munmap(fbp, screensize);
}
void YUVtoRGB(unsigned char *Y, unsigned char *U, unsigned char *V)
{
	int i, j, n = 0;

	r = (int *)malloc(sizeof(int)* width*height);
	g = (int *)malloc(sizeof(int)* width*height);
	b = (int *)malloc(sizeof(int)* width*height);

	for (j = 0; j < height*width; j++)
	{
		r[j] = 0;
		g[j] = 0;
		b[j] = 0;
	}
	
	for (j = 0; j < height; j = j + 1)
	{
		for (i = 0; i < width; i = i + 2)
		{
			r[i + (j*width)] = Y[i + (j*width)] + (1.13983*(V[n] - 128));
			r[i + (j*width) + 1] = Y[i + (j*width) + 1] + (1.13983*(V[n] - 128));

			g[i + (j*width)] = Y[i + (j*width)] - (0.39465*(U[n] - 128)) - (0.5806*(V[n] - 128));
			g[i + (j*width) + 1] = Y[i + (j*width) + width] - (0.39465*(U[n] - 128)) - (0.5806*(V[n] - 128));

			b[i + (j*width)] = Y[i + (j*width)] + (2.03211*(U[n] - 128));
			b[i + (j*width) + 1] = Y[i + (j*width) + 1] + (2.03211*(U[n] - 128));

			n++;
		}
	}
	for (j = 0; j < height; j++)
	{
		for (i = 0; i < width; i++)
		{
			R[i][j] = r[i + j*width];
			G[i][j] = g[i + j*width];
			B[i][j] = b[i + j*width];
		}
	}
	for (j = 0; j < height; j++)	//§PÂ_RGB¬O§_¦b½d³ò¤º
	{
		for (i = 0; i < width; i++)
		{
			if (R[i][j] > 255)	R[i][j] = 255;
			else if (R[i][j] < 0)	R[i][j] = 0;

			if (G[i][j] > 255)	G[i][j] = 255;
			else if (G[i][j] < 0)	G[i][j] = 0;

			if (B[i][j] > 255)	B[i][j] = 255;
			else if (B[i][j] < 0)	B[i][j] = 0;
		}
	}
	for (j = 0; j < height; j++)
	{
		for (i = 0; i < width; i++)
		{
			R2[i][height - j - 1] = R[i][j];
			G2[i][height - j - 1] = G[i][j];
			B2[i][height - j - 1] = B[i][j];
		}
	}
}
