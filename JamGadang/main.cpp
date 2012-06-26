//Tugas Besar Grafika Komputer "Pembuatan Jam Gadang"
//Imam Zainudin         10108380
//Dimas Ricky		    10108401
//Sani Surya S          10108416
//Hamdi Jamin           10109701
//Kurniawan             10106268

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifdef __APPLE__
#include <OpenGL/OpenGL.h>
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#include <GL/glu.h>
#include <GL/gl.h>
#include "imageloader.h"
#include "vec3f.h"
#endif

//static GLfloat spin, spin2 = 0.0;
float angle = 0;
float sudutk = 30.0f;
using namespace std;

float lastx, lasty;
GLint stencilBits;
static int viewx = 50;
static int viewy = 24;
static int viewz = 80;
int a=0, b=0, c=0, d=0;
float rot = 0;


//train 2D
//class untuk terain 2D
class Terrain {
private:
	int w; //Width
	int l; //Length
	float** hs; //Heights
	Vec3f** normals;
	bool computedNormals; //Whether normals is up-to-date
public:
	Terrain(int w2, int l2) {
		w = w2;
		l = l2;

		hs = new float*[l];
		for (int i = 0; i < l; i++) {
			hs[i] = new float[w];
		}

		normals = new Vec3f*[l];
		for (int i = 0; i < l; i++) {
			normals[i] = new Vec3f[w];
		}

		computedNormals = false;
	}

	~Terrain() {
		for (int i = 0; i < l; i++) {
			delete[] hs[i];
		}
		delete[] hs;

		for (int i = 0; i < l; i++) {
			delete[] normals[i];
		}
		delete[] normals;
	}

	int width() {
		return w;
	}

	int length() {
		return l;
	}

	//Sets the height at (x, z) to y
	void setHeight(int x, int z, float y) {
		hs[z][x] = y;
		computedNormals = false;
	}

	//Returns the height at (x, z)
	float getHeight(int x, int z) {
		return hs[z][x];
	}

	//Computes the normals, if they haven't been computed yet
	void computeNormals() {
		if (computedNormals) {
			return;
		}

		//Compute the rough version of the normals
		Vec3f** normals2 = new Vec3f*[l];
		for (int i = 0; i < l; i++) {
			normals2[i] = new Vec3f[w];
		}

		for (int z = 0; z < l; z++) {
			for (int x = 0; x < w; x++) {
				Vec3f sum(0.0f, 0.0f, 0.0f);

				Vec3f out;
				if (z > 0) {
					out = Vec3f(0.0f, hs[z - 1][x] - hs[z][x], -1.0f);
				}
				Vec3f in;
				if (z < l - 1) {
					in = Vec3f(0.0f, hs[z + 1][x] - hs[z][x], 1.0f);
				}
				Vec3f left;
				if (x > 0) {
					left = Vec3f(-1.0f, hs[z][x - 1] - hs[z][x], 0.0f);
				}
				Vec3f right;
				if (x < w - 1) {
					right = Vec3f(1.0f, hs[z][x + 1] - hs[z][x], 0.0f);
				}

				if (x > 0 && z > 0) {
					sum += out.cross(left).normalize();
				}
				if (x > 0 && z < l - 1) {
					sum += left.cross(in).normalize();
				}
				if (x < w - 1 && z < l - 1) {
					sum += in.cross(right).normalize();
				}
				if (x < w - 1 && z > 0) {
					sum += right.cross(out).normalize();
				}

				normals2[z][x] = sum;
			}
		}

		//Smooth out the normals
		const float FALLOUT_RATIO = 0.5f;
		for (int z = 0; z < l; z++) {
			for (int x = 0; x < w; x++) {
				Vec3f sum = normals2[z][x];

				if (x > 0) {
					sum += normals2[z][x - 1] * FALLOUT_RATIO;
				}
				if (x < w - 1) {
					sum += normals2[z][x + 1] * FALLOUT_RATIO;
				}
				if (z > 0) {
					sum += normals2[z - 1][x] * FALLOUT_RATIO;
				}
				if (z < l - 1) {
					sum += normals2[z + 1][x] * FALLOUT_RATIO;
				}

				if (sum.magnitude() == 0) {
					sum = Vec3f(0.0f, 1.0f, 0.0f);
				}
				normals[z][x] = sum;
			}
		}

		for (int i = 0; i < l; i++) {
			delete[] normals2[i];
		}
		delete[] normals2;

		computedNormals = true;
	}

	//Returns the normal at (x, z)
	Vec3f getNormal(int x, int z) {
		if (!computedNormals) {
			computeNormals();
		}
		return normals[z][x];
	}
};
//end class


void initRendering() {
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_NORMALIZE);
	glShadeModel(GL_SMOOTH);
}

//Loads a terrain from a heightmap.  The heights of the terrain range from
//-height / 2 to height / 2.
//load terain di procedure inisialisasi
Terrain* loadTerrain(const char* filename, float height) {
	Image* image = loadBMP(filename);
	Terrain* t = new Terrain(image->width, image->height);
	for (int y = 0; y < image->height; y++) {
		for (int x = 0; x < image->width; x++) {
			unsigned char color = (unsigned char) image->pixels[3 * (y
					* image->width + x)];
			float h = height * ((color / 255.0f) - 0.5f);
			t->setHeight(x, y, h);
		}
	}

	delete image;
	t->computeNormals();
	return t;
}

float _angle = 60.0f;
//buat tipe data terain
Terrain* _terrain;
Terrain* _terrainTanah;
Terrain* _terrainAir;

const GLfloat light_ambient[] = { 0.3f, 0.3f, 0.3f, 1.0f };
const GLfloat light_diffuse[] = { 0.7f, 0.7f, 0.7f, 1.0f };
const GLfloat light_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat light_position[] = { 1.0f, 1.0f, 1.0f, 1.0f };

const GLfloat light_ambient2[] = { 0.3f, 0.3f, 0.3f, 0.0f };
const GLfloat light_diffuse2[] = { 0.3f, 0.3f, 0.3f, 0.0f };

const GLfloat mat_ambient[] = { 0.8f, 0.8f, 0.8f, 1.0f };
const GLfloat mat_diffuse[] = { 0.8f, 0.8f, 0.8f, 1.0f };
const GLfloat mat_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat high_shininess[] = { 100.0f };

void cleanup() {
	delete _terrain;
	delete _terrainTanah;
}

//untuk di display
void drawSceneTanah(Terrain *terrain, GLfloat r, GLfloat g, GLfloat b) {
	//	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	/*
	 glMatrixMode(GL_MODELVIEW);
	 glLoadIdentity();
	 glTranslatef(0.0f, 0.0f, -10.0f);
	 glRotatef(30.0f, 1.0f, 0.0f, 0.0f);
	 glRotatef(-_angle, 0.0f, 1.0f, 0.0f);

	 GLfloat ambientColor[] = {0.4f, 0.4f, 0.4f, 1.0f};
	 glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientColor);

	 GLfloat lightColor0[] = {0.6f, 0.6f, 0.6f, 1.0f};
	 GLfloat lightPos0[] = {-0.5f, 0.8f, 0.1f, 0.0f};
	 glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColor0);
	 glLightfv(GL_LIGHT0, GL_POSITION, lightPos0);
	 */
	float scale = 150.0f / max(terrain->width() - 1, terrain->length() - 1);
	glScalef(scale, scale, scale);
	glTranslatef(-(float) (terrain->width() - 1) / 2, 0.0f,
			-(float) (terrain->length() - 1) / 2);

	glColor3f(r, g, b);
	for (int z = 0; z < terrain->length() - 1; z++) {
		//Makes OpenGL draw a triangle at every three consecutive vertices
		glBegin(GL_TRIANGLE_STRIP);
		for (int x = 0; x < terrain->width(); x++) {
			Vec3f normal = terrain->getNormal(x, z);
			glNormal3f(normal[0], normal[1], normal[2]);
			glVertex3f(x, terrain->getHeight(x, z), z);
			normal = terrain->getNormal(x, z + 1);
			glNormal3f(normal[0], normal[1], normal[2]);
			glVertex3f(x, terrain->getHeight(x, z + 1), z + 1);
		}
		glEnd();
	}

}

//MARKA JALAN
void marka()
{
    glPushMatrix();
	glColor3f(1.0f, 1.0f, 1.0f);
    glScaled(10.5, 0.1 , 2.5);
    glutSolidCube(0.5f);
    glPopMatrix();
}
//LAMPU
void lampuJalan()
{
   //Tiang Tegak
	glPushMatrix();
	glColor3f(0.5, 0.5, 0.5);
	glScalef(0.04,1.7,0.05);
	glutSolidCube(7.0f);
	glPopMatrix();

    //Tiang Atas
	glPushMatrix();
	glColor3f(0.5f, 0.5f, 0.5f);
	glTranslatef(0.0,5.3,-2.0);
    glScaled(0.5, 1.0 , 7.5);
    glutSolidCube(0.5f);
	glPopMatrix();

	//Lampu
	glPushMatrix();
	glTranslatef(0.0, 4.7, -3.7);
	glColor3f(1, 1, 1);
	glScalef(0.8,0.8,1.5);
	glutSolidSphere(0.5,70,20);
	glPopMatrix();

}

void lampuJalan2()
{
   //Tiang Tegak
	glPushMatrix();
	glColor3f(0.5, 0.5, 0.5);
	glScalef(0.04,1.7,0.05);
	glutSolidCube(7.0f);
	glPopMatrix();

    //Tiang Atas
	glPushMatrix();
	glColor3f(0.5f, 0.5f, 0.5f);
	glTranslatef(0.0,5.3,2.0);
    glScaled(0.5, 1.0 , 7.5);
    glutSolidCube(0.5f);
	glPopMatrix();

	//Lampu
	glPushMatrix();
	glTranslatef(0.0, 4.7, 3.7);
	glColor3f(1, 1, 1);
	glScalef(0.8,0.8,1.5);
	glutSolidSphere(0.5,70,20);
	glPopMatrix();

}

void lampu()
{
    //Tiang
	glPushMatrix();
	glColor4f(0.5, 0.5, 0.5, 0.0);
	glScalef(0.02,0.3,0.04);
	glutSolidCube(7.0f);
	glPopMatrix();

	//Lampu
	glPushMatrix();
	glTranslatef(0.0f, 2.0f, 0.0f);
	glColor3f(1, 1, 1);
	glScalef(2.0,2.0,3.0);
	glutSolidSphere(0.5,70,20);
	glPopMatrix();
}

//PAGAR
void pagar()
{
    //Pagar Atas
    glPushMatrix();
	glColor4f(0.5f, 0.5f, 0.5f, 1.0f);
    glTranslatef(-26.0f, 2.0f, 0.0f);
    glScaled(297.0, 1.0 , 0.5);
    glutSolidCube(0.5f);
    glPopMatrix();

    //Pagar Bawah
    glPushMatrix();
	glColor4f(0.5f, 0.5f, 0.5f, 1.0f);
    glTranslatef(-26.0f, 1.05f, 0.0f);
    glScaled(297.0, 1.0 , 0.5);
    glutSolidCube(0.5f);
    glPopMatrix();

    //Pagar Tegak
    glPushMatrix();
	glColor4f(0.5f, 0.5f, 0.5f, 1.0f);
    glTranslatef(1.0f, 0.0f, 0.0f);
    glScaled(1.5, 10.0 , 0.5);
    glutSolidCube(0.5f);
    glPopMatrix();

    glPushMatrix();
	glColor4f(0.5f, 0.5f, 0.5f, 1.0f);
    glTranslatef(26.0f, 0.0f, 0.0f);
    glScaled(1.5, 10.0 , 0.5);
    glutSolidCube(0.5f);
    glPopMatrix();

    glPushMatrix();
	glColor4f(0.5f, 0.5f, 0.5f, 1.0f);
    glTranslatef(47.8f, 0.0f, 0.0f);
    glScaled(1.5, 10.0 , 0.5);
    glutSolidCube(0.5f);
    glPopMatrix();

    glPushMatrix();
	glColor4f(0.5f, 0.5f, 0.5f, 1.0f);
    glTranslatef(-26.0f, 0.0f, 0.0f);
    glScaled(1.5, 10.0 , 0.5);
    glutSolidCube(0.5f);
    glPopMatrix();

    glPushMatrix();
	glColor4f(0.5f, 0.5f, 0.5f, 1.0f);
    glTranslatef(-51.0f, 0.0f, 0.0f);
    glScaled(1.5, 10.0 , 0.5);
    glutSolidCube(0.5f);
    glPopMatrix();

    glPushMatrix();
	glColor4f(0.5f, 0.5f, 0.5f, 1.0f);
    glTranslatef(-76.0f, 0.0f, 0.0f);
    glScaled(1.5, 10.0 , 0.5);
    glutSolidCube(0.5f);
    glPopMatrix();

    glPushMatrix();
	glColor4f(0.5f, 0.5f, 0.5f, 1.0f);
    glTranslatef(-100.0f, 0.0f, 0.0f);
    glScaled(1.5, 10.0 , 0.5);
    glutSolidCube(0.5f);
    glPopMatrix();
}

//LANTAI BANGUNAN
void bangunan()
{
    //Dasar
    glPushMatrix();
	glColor4f(0.5f, 0.5f, 0.5f, 1.0f);
	glScalef(4.0,0.02,4.5);
	glutSolidCube(9.0f);
    glPopMatrix();

    //Dasar2
    glPushMatrix();
    glColor3f(0.0, 0.0, 0.0);
    glTranslatef(0.0, 0.0, 28.5);
    glScalef(1.5,0.02,1.9);
	glutSolidCube(9.0f);
    glPopMatrix();

    //Trotoar kanan
    glPushMatrix();
    glColor3f(0.5, 0.5, 0.5);
    glTranslatef(7.0, 0.0, 28.5);
    glScalef(0.05,0.1,1.9);
	glutSolidCube(9.0f);
    glPopMatrix();
    //Trotoar Kiri
    glPushMatrix();
    glColor3f(0.5, 0.5, 0.5);
    glTranslatef(-7.0, 0.0, 28.5);
    glScalef(0.05,0.1,1.9);
	glutSolidCube(9.0f);
    glPopMatrix();

//Plang
    //Tiang kanan
    glPushMatrix();
	glColor4f(0.5f, 0.5f, 0.5f, 1.0f);
    glTranslatef(7.0, 5.0, 36.0);
    glScaled(1.0, 20.0 , 0.5);
    glutSolidCube(0.5f);
    glPopMatrix();
    //Tiang kiri
    glPushMatrix();
	glColor4f(0.5f, 0.5f, 0.5f, 1.0f);
    glTranslatef(-7.0, 5.0, 36.0);
    glScaled(1.0, 20.0 , 0.5);
    glutSolidCube(0.5f);
    glPopMatrix();
    //Plang
    glPushMatrix();
	glColor4f(0.8f, 0.5f, 0.0f, 1.0f);
    glTranslatef(0.0, 10.0, 36.0);
    glScaled(30.0, 5.0 , 0.5);
    glutSolidCube(0.5f);
    glPopMatrix();

}

//BANGUN JAM GADANG
void lantai1()
{
	glPushMatrix();
	glTranslatef(0.0f, -3.3f, 0.0f);
	glColor4f(0.8, 0.8, 0.8, 0.8);
	glScalef(1.8,1.4,1.8);
	glutSolidCube(6.0f);
	glPopMatrix();

}

void jendelalantai1()
{
	glPushMatrix();
	glTranslatef(-1.5f, -3.6f, 0.0f);
	glColor4f(0.0, 0.0, 0.0, 0.0);
	glScalef(0.8,1.3,3.7);
	glutSolidCube(3.0f);
	glPopMatrix();

    glPushMatrix();
	glTranslatef(1.5f, -3.6f, 0.0f);
	glColor4f(0.0, 0.0, 0.0, 0.0);
	glScalef(0.8,1.3,3.7);
	glutSolidCube(3.0f);
	glPopMatrix();
}

void tangga()
{
    glPushMatrix();
	glTranslatef(0.0f, -1.5f, 3.0f);
	glColor4f(0.8, 0.8, 0.8, 0.8);
	glScalef(1.795,0.09,1.795);
	glutSolidCube(6.0f);
	glPopMatrix();

	//tangga1
	glPushMatrix();
	glTranslatef(0.0f, -1.1f, 3.0f);
	glColor4f(0.8, 0.8, 0.8, 0.8);
	glScalef(0.7,0.08,1.795);
	glutSolidCube(6.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.0f, -0.68f, 3.0f);
	glColor4f(0.8, 0.8, 0.8, 0.8);
	glScalef(0.5,0.08,1.795);
	glutSolidCube(6.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.0f, -0.3f, 3.0f);
	glColor4f(0.8, 0.8, 0.8, 0.8);
	glScalef(0.3,0.08,1.795);
	glutSolidCube(6.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.0f, 0.0f, 2.0f);
	glColor4f(0.8, 0.8, 0.8, 0.8);
	glScalef(0.3,0.08,1.795);
	glutSolidCube(6.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.0f, 0.3f, 1.5f);
	glColor4f(0.8, 0.8, 0.8, 0.8);
	glScalef(0.3,0.08,1.795);
	glutSolidCube(6.0f);
	glPopMatrix();

	//tangga kiri & kanan
	glPushMatrix();
	glTranslatef(-3.745f, -1.99f, 3.7f);
	glColor4f(0.8, 0.8, 0.8, 0.8);
	glScalef(0.55,0.1,1.795);
	glutSolidCube(6.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.0f, -1.99f, 3.0f);
	glColor4f(0.8, 0.8, 0.8, 0.8);
	glScalef(0.3,0.1,1.795);
	glutSolidCube(6.0f);
	glPopMatrix();

    glPushMatrix();
	glTranslatef(0.0f, -4.09f, 2.0f);
	glColor4f(0.8, 0.8, 0.8, 0.8);
	glScalef(0.1,0.8,1.795);
	glutSolidCube(6.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(3.745f, -1.99f, 3.7f);
	glColor4f(0.8, 0.8, 0.8, 0.8);
	glScalef(0.55,0.1,1.795);
	glutSolidCube(6.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-4.045f, -2.58f, 4.5f);
	glColor4f(0.8, 0.8, 0.8, 0.8);
	glScalef(0.45,0.1,1.795);
	glutSolidCube(6.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(4.045f, -2.58f, 4.5f);
	glColor4f(0.8, 0.8, 0.8, 0.8);
	glScalef(0.45,0.1,1.795);
	glutSolidCube(6.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-4.045f, -2.98f, 5.3f);
	glColor4f(0.8, 0.8, 0.8, 0.8);
	glScalef(0.45,0.1,1.795);
	glutSolidCube(6.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(4.045f, -2.98f, 5.3f);
	glColor4f(0.8, 0.8, 0.8, 0.8);
	glScalef(0.45,0.1,1.795);
	glutSolidCube(6.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-4.045f, -3.38f, 6.0f);
	glColor4f(0.8, 0.8, 0.8, 0.8);
	glScalef(0.45,0.1,1.795);
	glutSolidCube(6.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(4.045f, -3.38f, 6.0f);
	glColor4f(0.8, 0.8, 0.8, 0.8);
	glScalef(0.45,0.1,1.795);
	glutSolidCube(6.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-4.045f, -3.78f, 6.7f);
	glColor4f(0.8, 0.8, 0.8, 0.8);
	glScalef(0.45,0.1,1.795);
	glutSolidCube(6.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(4.045f, -3.78f, 6.7f);
	glColor4f(0.8, 0.8, 0.8, 0.8);
	glScalef(0.45,0.1,1.795);
	glutSolidCube(6.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-4.045f, -4.18f, 7.4f);
	glColor4f(0.8, 0.8, 0.8, 0.8);
	glScalef(0.45,0.1,1.795);
	glutSolidCube(6.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(4.045f, -4.18f, 7.4f);
	glColor4f(0.8, 0.8, 0.8, 0.8);
	glScalef(0.45,0.1,1.795);
	glutSolidCube(6.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-4.045f, -4.58f, 8.1f);
	glColor4f(0.8, 0.8, 0.8, 0.8);
	glScalef(0.45,0.1,1.795);
	glutSolidCube(6.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(4.045f, -4.58f, 8.1f);
	glColor4f(0.8, 0.8, 0.8, 0.8);
	glScalef(0.45,0.1,1.795);
	glutSolidCube(6.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-4.045f, -4.98f, 8.8f);
	glColor4f(0.8, 0.8, 0.8, 0.8);
	glScalef(0.45,0.1,1.795);
	glutSolidCube(6.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(4.045f, -4.98f, 8.8f);
	glColor4f(0.8, 0.8, 0.8, 0.8);
	glScalef(0.45,0.1,1.795);
	glutSolidCube(6.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-4.045f, -5.38f, 9.5f);
	glColor4f(0.8, 0.8, 0.8, 0.8);
	glScalef(0.45,0.1,1.795);
	glutSolidCube(6.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(4.045f, -5.38f, 9.5f);
	glColor4f(0.8, 0.8, 0.8, 0.8);
	glScalef(0.45,0.1,1.795);
	glutSolidCube(6.0f);
	glPopMatrix();
}

void pagartangga()
{
    //tiang pagar bawah
    glPushMatrix();
	glTranslatef(5.2f, -4.3f, 14.5f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.2,1.85,0.2);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-5.2f, -4.3f, 14.5f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.2,1.85,0.2);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(2.9f, -4.3f, 14.5f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.2,1.85,0.2);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-2.9f, -4.3f, 14.5f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.2,1.85,0.2);
	glutSolidCube(1.0f);
	glPopMatrix();

	//tiang pagar tengah
	glPushMatrix();
	glTranslatef(5.2f, -2.8f, 11.8f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.2,1.85,0.2);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-5.2f, -2.8f, 11.8f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.2,1.85,0.2);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(2.9f, -2.8f, 11.8f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.2,1.85,0.2);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-2.9f, -2.8f, 11.8f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.2,1.85,0.2);
	glutSolidCube(1.0f);
	glPopMatrix();

	//tiang pagar atas
	glPushMatrix();
	glTranslatef(5.2f, -0.5f, 8.2f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.2,1.85,0.2);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-5.2f, -0.5f, 8.2f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.2,1.85,0.2);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(2.9f, -0.5f, 8.2f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.2,1.85,0.2);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-2.9f, -0.5f, 8.2f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.2,1.85,0.2);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.0f, 0.5f, 8.2f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(1.5,1.85,0.2);
	glutSolidCube(1.0f);
	glPopMatrix();

	//pegangan pagar
	glPushMatrix();
	glTranslatef(5.2f, -1.6f, 11.4f);
	glRotatef(32,1,0,0);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.2,0.2,7.6);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-5.2f, -1.6f, 11.4f);
	glRotatef(32,1,0,0);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.2,0.2,7.6);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(2.9f, -1.6f, 11.4f);
	glRotatef(32,1,0,0);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.2,0.2,7.6);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-2.9f, -1.6f, 11.4f);
	glRotatef(32,1,0,0);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.2,0.2,7.6);
	glutSolidCube(1.0f);
	glPopMatrix();

	//pegangan pagar atas
	glPushMatrix();
	glTranslatef(5.2f, 0.4f, 6.0f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.2,0.2,4.5);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-5.2f, 0.4f, 6.0f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.2,0.2,4.5);
	glutSolidCube(1.0f);
	glPopMatrix();

	//pegangan tangga tengah
	glPushMatrix();
	glTranslatef(1.7f, 0.8f, 8.2f);
	glRotatef(-23,0,0,1);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(2.8,0.2,0.2);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-1.7f, 0.8f, 8.2f);
	glRotatef(23,0,0,1);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(2.8,0.2,0.2);
	glutSolidCube(1.0f);
	glPopMatrix();

}

void lantai2()
{
	glPushMatrix();
	glTranslatef(0.0f, 3.2f, 0.0f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(1.2,1.8,1.2);
	glutSolidCube(6.0f);
	glPopMatrix();

}

void jendelalantai2()
{
    //depan belakang
	glPushMatrix();
	glTranslatef(-1.8f, 4.8f, 0.0f);
	glColor4f(0.0, 0.0, 0.0, 0.0);
	glScalef(0.4,1.85,2.47);
	glutSolidCube(3.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.0f, 4.8f, 0.0f);
	glColor4f(0.0, 0.0, 0.0, 0.0);
	glScalef(0.4,1.85,2.47);
	glutSolidCube(3.0f);
	glPopMatrix();

    glPushMatrix();
	glTranslatef(1.8f, 4.8f, 0.0f);
	glColor4f(0.0, 0.0, 0.0, 0.0);
	glScalef(0.4,1.85,2.47);
	glutSolidCube(3.0f);
	glPopMatrix();

	//relief tiang
	glPushMatrix();
	glTranslatef(3.1f, 4.8f, 0.0f);
	glColor4f(0.0, 0.0, 0.0, 0.0);
	glScalef(0.18,1.9,2.45);
	glutWireCube(3.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-3.1f, 4.8f, 0.0f);
	glColor4f(0.0, 0.0, 0.0, 0.0);
	glScalef(0.18,1.9,2.45);
	glutWireCube(3.0f);
	glPopMatrix();

    glPushMatrix();
	glTranslatef(0.0f, 4.8f, 3.1f);
	glColor4f(0.0, 0.0, 0.0, 0.0);
	glScalef(2.45,1.9,0.18);
	glutWireCube(3.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.0f, 4.8f, -3.1f);
	glColor4f(0.0, 0.0, 0.0, 0.0);
	glScalef(2.45,1.9,0.18);
	glutWireCube(3.0f);
	glPopMatrix();

	//kiri kanan
    glPushMatrix();
	glTranslatef(0.0f, 4.8f, 0.0f);
	glColor4f(0.0, 0.0, 0.0, 0.0);
	glScalef(2.47,1.85,0.4);
	glutSolidCube(3.0f);
	glPopMatrix();

    glPushMatrix();
	glTranslatef(0.0f, 4.8f, 1.8f);
	glColor4f(0.0, 0.0, 0.0, 0.0);
	glScalef(2.47,1.85,0.4);
	glutSolidCube(3.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.0f, 4.8f, -1.8f);
	glColor4f(0.0, 0.0, 0.0, 0.0);
	glScalef(2.47,1.85,0.4);
	glutSolidCube(3.0f);
	glPopMatrix();
}

void pagarlantai2()
{
    //depan kiri kanan
    glPushMatrix();
	glTranslatef(5.2f, 1.3f, 5.2f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.3,1.85,0.3);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(4.2f, 1.3f, 5.2f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.3,1.85,0.3);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(3.2f, 2.08f, 5.2f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(4.3,0.3,0.4);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(3.2f, 1.3f, 5.2f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.3,1.85,0.3);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(2.2f, 1.3f, 5.2f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.3,1.85,0.3);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(1.2f, 1.3f, 5.2f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.3,1.85,0.3);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-1.2f, 1.3f, 5.2f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.3,1.85,0.3);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-2.2f, 1.3f, 5.2f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.3,1.85,0.3);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-3.2f, 1.3f, 5.2f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.3,1.85,0.3);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-3.2f, 2.08f, 5.2f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(4.3,0.3,0.4);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-4.2f, 1.3f, 5.2f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.3,1.85,0.3);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-5.2f, 1.3f, 5.2f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.3,1.85,0.3);
	glutSolidCube(1.0f);
	glPopMatrix();

	//kiri
	glPushMatrix();
	glTranslatef(-5.2f, 1.3f, -5.2f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.3,1.85,0.3);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-5.2f, 1.3f, -4.2f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.3,1.85,0.3);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-5.2f, 1.3f, -3.2f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.3,1.85,0.3);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-5.2f, 1.3f, -2.2f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.3,1.85,0.3);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-5.2f, 1.3f, -1.2f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.3,1.85,0.3);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-5.2f, 1.3f, 0.0f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.3,1.85,0.3);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-5.2f, 2.08f, 0.0f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.4,0.3,10.5);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-5.2f, 1.3f, 1.2f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.3,1.85,0.3);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-5.2f, 1.3f, 2.2f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.3,1.85,0.3);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-5.2f, 1.3f, 3.2f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.3,1.85,0.3);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-5.2f, 1.3f, 4.2f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.3,1.85,0.3);
	glutSolidCube(1.0f);
	glPopMatrix();

	//kanan
	glPushMatrix();
	glTranslatef(5.2f, 1.3f, -5.2f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.3,1.85,0.3);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(5.2f, 1.3f, -4.2f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.3,1.85,0.3);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(5.2f, 1.3f, -3.2f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.3,1.85,0.3);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(5.2f, 1.3f, -2.2f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.3,1.85,0.3);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(5.2f, 1.3f, -1.2f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.3,1.85,0.3);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(5.2f, 1.3f, 0.0f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.3,1.85,0.3);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(5.2f, 2.08f, 0.0f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.4,0.3,10.5);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(5.2f, 1.3f, 1.2f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.3,1.85,0.3);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(5.2f, 1.3f, 2.2f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.3,1.85,0.3);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(5.2f, 1.3f, 3.2f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.3,1.85,0.3);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(5.2f, 1.3f, 4.2f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.3,1.85,0.3);
	glutSolidCube(1.0f);
	glPopMatrix();

	//belakang
	glPushMatrix();
	glTranslatef(4.2f, 1.3f, -5.2f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.3,1.85,0.3);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(3.2f, 1.3f, -5.2f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.3,1.85,0.3);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(2.2f, 1.3f, -5.2f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.3,1.85,0.3);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(1.2f, 1.3f, -5.2f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.3,1.85,0.3);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.0f, 1.3f, -5.2f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.3,1.85,0.3);
	glutSolidCube(1.0f);
	glPopMatrix();

    glPushMatrix();
	glTranslatef(0.0f, 2.08f, -5.2f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(10.5,0.3,0.4);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-1.2f, 1.3f, -5.2f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.3,1.85,0.3);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-2.2f, 1.3f, -5.2f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.3,1.85,0.3);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-3.2f, 1.3f, -5.2f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.3,1.85,0.3);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-4.2f, 1.3f, -5.2f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.3,1.85,0.3);
	glutSolidCube(1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-5.2f, 1.3f, -5.2f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.3,1.85,0.3);
	glutSolidCube(1.0f);
	glPopMatrix();
}

void lantai3()
{
	glPushMatrix();
	glTranslatef(0.0f, 11.5f, 0.0f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(1.1,1.5,1.1);
	glutSolidCube(6.0f);
	glPopMatrix();

    glPushMatrix();
	glTranslatef(0.0f, 8.6f, 0.0f);
	glColor4f(0.8, 0.8, 0.8, 0.8);
	glScalef(1.28,0.08,1.28);
	glutSolidCube(6.0f);
	glPopMatrix();

}

void jendelalantai3()
{
    //depan belakang
	glPushMatrix();
	glTranslatef(-1.7f, 12.2f, 0.0f);
	glColor4f(0.0, 0.0, 0.0, 0.0);
	glScalef(0.35,1.8,2.25);
	glutSolidCube(3.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.0f, 12.2f, 0.0f);
	glColor4f(0.0, 0.0, 0.0, 0.0);
	glScalef(0.35,1.8,2.25);
	glutSolidCube(3.0f);
	glPopMatrix();

    glPushMatrix();
	glTranslatef(1.7f, 12.2f, 0.0f);
	glColor4f(0.0, 0.0, 0.0, 0.0);
	glScalef(0.35,1.8,2.25);
	glutSolidCube(3.0f);
	glPopMatrix();

	//relief tiang
	glPushMatrix();
	glTranslatef(2.8f, 12.2f, 0.0f);
	glColor4f(0.0, 0.0, 0.0, 0.0);
	glScalef(0.19,2.0,2.25);
	glutWireCube(3.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-2.8f, 12.2f, 0.0f);
	glColor4f(0.0, 0.0, 0.0, 0.0);
	glScalef(0.19,2.0,2.25);
	glutWireCube(3.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.0f, 12.2f, 2.8f);
	glColor4f(0.0, 0.0, 0.0, 0.0);
	glScalef(2.25,2.0,0.19);
	glutWireCube(3.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.0f, 12.2f, -2.8f);
	glColor4f(0.0, 0.0, 0.0, 0.0);
	glScalef(2.25,2.0,0.19);
	glutWireCube(3.0f);
	glPopMatrix();

	//kiri kanan
    glPushMatrix();
	glTranslatef(0.0f, 12.2f, 0.0f);
	glColor4f(0.0, 0.0, 0.0, 0.0);
	glScalef(2.25,1.8,0.35);
	glutSolidCube(3.0f);
	glPopMatrix();

    glPushMatrix();
	glTranslatef(0.0f, 12.2f, 1.7f);
	glColor4f(0.0, 0.0, 0.0, 0.0);
	glScalef(2.25,1.8,0.35);
	glutSolidCube(3.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.0f, 12.2f, -1.7f);
	glColor4f(0.0, 0.0, 0.0, 0.0);
	glScalef(2.25,1.8,0.35);
	glutSolidCube(3.0f);
	glPopMatrix();
}

void lantai4()
{
	glPushMatrix();
	glTranslatef(0.0f, 19.0f, 0.0f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(1.0,1.2,1.0);
	glutSolidCube(6.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.0f, 16.0f, 0.0f);
	glColor4f(0.8, 0.8, 0.8, 0.8);
	glScalef(1.17,0.08,1.17);
	glutSolidCube(6.0f);
	glPopMatrix();

}

void jendelalantai4()
{
    //depan belakang
	glPushMatrix();
	glTranslatef(-1.2f, 19.4f, 0.0f);
	glColor4f(0.0, 0.0, 0.0, 0.0);
	glScalef(0.5,1.4,2.08);
	glutSolidCube(3.0f);
	glPopMatrix();

    glPushMatrix();
	glTranslatef(1.2f, 19.4f, 0.0f);
	glColor4f(0.0, 0.0, 0.0, 0.0);
	glScalef(0.5,1.4,2.08);
	glutSolidCube(3.0f);
	glPopMatrix();

	//relief tiang
    glPushMatrix();
	glTranslatef(-2.5f, 19.4f, 0.0f);
	glColor4f(0.0, 0.0, 0.0, 0.0);
	glScalef(0.18,1.9,2.08);
	glutWireCube(3.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(2.5f, 19.4f, 0.0f);
	glColor4f(0.0, 0.0, 0.0, 0.0);
	glScalef(0.18,1.9,2.08);
	glutWireCube(3.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.0f, 19.4f, -2.5f);
	glColor4f(0.0, 0.0, 0.0, 0.0);
	glScalef(2.08,1.9,0.18);
	glutWireCube(3.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.0f, 19.4f, 2.5f);
	glColor4f(0.0, 0.0, 0.0, 0.0);
	glScalef(2.08,1.9,0.18);
	glutWireCube(3.0f);
	glPopMatrix();

	//kiri kanan
    glPushMatrix();
	glTranslatef(0.0f, 19.4f, 1.2f);
	glColor4f(0.0, 0.0, 0.0, 0.0);
	glScalef(2.08,1.4,0.5);
	glutSolidCube(3.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.0f, 19.4f, -1.2f);
	glColor4f(0.0, 0.0, 0.0, 0.0);
	glScalef(2.08,1.4,0.5);
	glutSolidCube(3.0f);
	glPopMatrix();
}

void lantai5()
{
	glPushMatrix();
	glTranslatef(0.0f, 25.5f, 0.0f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.89,1.0,0.89);
	glutSolidCube(6.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.0f, 22.8f, 0.0f);
	glColor4f(0.8, 0.8, 0.8, 0.8);
	glScalef(1.07,0.08,1.07);
	glutSolidCube(6.0f);
	glPopMatrix();

	//relief tiang
	glPushMatrix();
	glTranslatef(-2.3f, 25.6f, 0.0f);
	glColor4f(0.0, 0.0, 0.0, 0.0);
	glScalef(0.1,1.5,1.8);
	glutWireCube(3.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(2.3f, 25.6f, 0.0f);
	glColor4f(0.0, 0.0, 0.0, 0.0);
	glScalef(0.1,1.5,1.8);
	glutWireCube(3.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.0f, 25.6f, -2.3f);
	glColor4f(0.0, 0.0, 0.0, 0.0);
	glScalef(1.8,1.5,0.1);
	glutWireCube(3.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.0f, 25.6f, 2.3f);
	glColor4f(0.0, 0.0, 0.0, 0.0);
	glScalef(1.8,1.5,0.1);
	glutWireCube(3.0f);
	glPopMatrix();

	//relief dinding
	glPushMatrix();
	glTranslatef(-1.0f, 24.5f, 0.0f);
	glColor4f(0.0, 0.0, 0.0, 0.0);
	glScalef(0.38,0.5,1.8);
	glutWireCube(3.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(1.0f, 24.5f, 0.0f);
	glColor4f(0.0, 0.0, 0.0, 0.0);
	glScalef(0.38,0.5,1.8);
	glutWireCube(3.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.0f, 24.5f, 1.0f);
	glColor4f(0.0, 0.0, 0.0, 0.0);
	glScalef(1.8,0.5,0.38);
	glutWireCube(3.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.0f, 24.5f, -1.0f);
	glColor4f(0.0, 0.0, 0.0, 0.0);
	glScalef(1.8,0.5,0.38);
	glutWireCube(3.0f);
	glPopMatrix();
}

void jam()
{
    //depan belakang
	glPushMatrix();
	glTranslatef(0.0f, 26.5f, 0.0f);
	glColor4f(0.0, 0.0, 0.0, 0.0);
	glScalef(1.4,1.4,27.4);
	glutWireTorus(0.1f, 0.8f, 12.0f, 12.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.0f, 26.5f, 0.0f);
	glRotatef(90, 5, 260, 0);
	glColor4f(0.0, 0.0, 0.0, 0.0);
	glScalef(1.4,1.4,27.4);
	glutWireTorus(0.1f, 0.8f, 12.0f, 12.0f);
	glPopMatrix();

	//jarum jam depan
	glPushMatrix();
	glTranslatef(0.0f, 26.8f, 2.6f);
	glColor4f(0.0, 0.0, 0.0, 0.0);
	glScalef(0.01,0.3,0.1);
	glutWireCube(3.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-0.3f, 26.4f, 2.6f);
	glColor4f(0.0, 0.0, 0.0, 0.0);
	glScalef(0.3,0.01,0.1);
	glutWireCube(3.0f);
	glPopMatrix();

	//jarum jam belakang
	glPushMatrix();
	glTranslatef(0.0f, 26.8f, -2.6f);
	glColor4f(0.0, 0.0, 0.0, 0.0);
	glScalef(0.01,0.3,0.1);
	glutWireCube(3.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.3f, 26.4f, -2.6f);
	glColor4f(0.0, 0.0, 0.0, 0.0);
	glScalef(0.3,0.01,0.1);
	glutWireCube(3.0f);
	glPopMatrix();

	//jarum jam kanan
	glPushMatrix();
	glTranslatef(2.6f, 26.8f, 0.0f);
	glColor4f(0.0, 0.0, 0.0, 0.0);
	glScalef(0.1,0.3,0.01);
	glutWireCube(3.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(2.6f, 26.4f, 0.3f);
	glColor4f(0.0, 0.0, 0.0, 0.0);
	glScalef(0.1,0.01,0.3);
	glutWireCube(3.0f);
	glPopMatrix();

	//jarum jam kiri
	glPushMatrix();
	glTranslatef(-2.6f, 26.8f, 0.0f);
	glColor4f(0.0, 0.0, 0.0, 0.0);
	glScalef(0.1,0.3,0.01);
	glutWireCube(3.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-2.6f, 26.4f, -0.3f);
	glColor4f(0.0, 0.0, 0.0, 0.0);
	glScalef(0.1,0.01,0.3);
	glutWireCube(3.0f);
	glPopMatrix();

	//
}

void lantaiatas()
{
    glPushMatrix();
	glTranslatef(0.0f, 30.0f, 0.0f);
	glColor4f(2.0, 2.0, 2.0, 2.0);
	glScalef(0.7,0.8,0.7);
	glutSolidCube(6.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.0f, 28.5f, 0.0f);
	glColor4f(0.8, 0.8, 0.8, 0.8);
	glScalef(1.2,0.08,1.2);
	glutSolidCube(6.0f);
	glPopMatrix();

	//pagar depan
	glPushMatrix();
	glTranslatef(3.5f, 29.0f, 3.5f);
	glColor4f(0.8, 0.8, 0.8, 0.8);
	glScalef(0.05,0.5,0.05);
	glutSolidCube(3.0f);
    glPopMatrix();

    glPushMatrix();
	glTranslatef(1.25f, 29.0f, 3.5f);
	glColor4f(0.8, 0.8, 0.8, 0.8);
	glScalef(0.05,0.5,0.05);
	glutSolidCube(3.0f);
    glPopMatrix();

    glPushMatrix();
	glTranslatef(0.0f, 29.2f, 3.5f);
	glColor4f(0.8, 0.8, 0.8, 0.8);
	glScalef(2.4,0.05,0.05);
	glutSolidCube(3.0f);
    glPopMatrix();

    glPushMatrix();
	glTranslatef(0.0f, 29.7f, 3.5f);
	glColor4f(0.8, 0.8, 0.8, 0.8);
	glScalef(2.4,0.05,0.05);
	glutSolidCube(3.0f);
    glPopMatrix();

    glPushMatrix();
	glTranslatef(-1.25f, 29.0f, 3.5f);
	glColor4f(0.8, 0.8, 0.8, 0.8);
	glScalef(0.05,0.5,0.05);
	glutSolidCube(3.0f);
    glPopMatrix();

    glPushMatrix();
	glTranslatef(-3.5f, 29.0f, 3.5f);
	glColor4f(0.8, 0.8, 0.8, 0.8);
	glScalef(0.05,0.5,0.05);
	glutSolidCube(3.0f);
    glPopMatrix();

    //pagar belakang
    glPushMatrix();
	glTranslatef(3.5f, 29.0f, -3.5f);
	glColor4f(0.8, 0.8, 0.8, 0.8);
	glScalef(0.05,0.5,0.05);
	glutSolidCube(3.0f);
    glPopMatrix();

    glPushMatrix();
	glTranslatef(1.25f, 29.0f, -3.5f);
	glColor4f(0.8, 0.8, 0.8, 0.8);
	glScalef(0.05,0.5,0.05);
	glutSolidCube(3.0f);
    glPopMatrix();

    glPushMatrix();
	glTranslatef(0.0f, 29.7f, -3.5f);
	glColor4f(0.8, 0.8, 0.8, 0.8);
	glScalef(2.4,0.05,0.05);
	glutSolidCube(3.0f);
    glPopMatrix();

    glPushMatrix();
	glTranslatef(0.0f, 29.2f, -3.5f);
	glColor4f(0.8, 0.8, 0.8, 0.8);
	glScalef(2.4,0.05,0.05);
	glutSolidCube(3.0f);
    glPopMatrix();

    glPushMatrix();
	glTranslatef(-1.25f, 29.0f, -3.5f);
	glColor4f(0.8, 0.8, 0.8, 0.8);
	glScalef(0.05,0.5,0.05);
	glutSolidCube(3.0f);
    glPopMatrix();

    glPushMatrix();
	glTranslatef(-3.5f, 29.0f, -3.5f);
	glColor4f(0.8, 0.8, 0.8, 0.8);
	glScalef(0.05,0.5,0.05);
	glutSolidCube(3.0f);
    glPopMatrix();

    //pagar kanan
    glPushMatrix();
	glTranslatef(3.5f, 29.0f, 1.25f);
	glColor4f(0.8, 0.8, 0.8, 0.8);
	glScalef(0.05,0.5,0.05);
	glutSolidCube(3.0f);
    glPopMatrix();

    glPushMatrix();
	glTranslatef(3.5f, 29.2f, 0.0f);
	glColor4f(0.8, 0.8, 0.8, 0.8);
	glScalef(0.05,0.05,2.4);
	glutSolidCube(3.0f);
    glPopMatrix();

    glPushMatrix();
	glTranslatef(3.5f, 29.7f, -0.0f);
	glColor4f(0.8, 0.8, 0.8, 0.8);
	glScalef(0.05,0.05,2.4);
	glutSolidCube(3.0f);
    glPopMatrix();

    glPushMatrix();
	glTranslatef(3.5f, 29.0f, -1.25f);
	glColor4f(0.8, 0.8, 0.8, 0.8);
	glScalef(0.05,0.5,0.05);
	glutSolidCube(3.0f);
    glPopMatrix();

    //pagar kiri
    glPushMatrix();
	glTranslatef(-3.5f, 29.0f, 1.25f);
	glColor4f(0.8, 0.8, 0.8, 0.8);
	glScalef(0.05,0.5,0.05);
	glutSolidCube(3.0f);
    glPopMatrix();

    glPushMatrix();
	glTranslatef(-3.5f, 29.2f, 0.0f);
	glColor4f(0.8, 0.8, 0.8, 0.8);
	glScalef(0.05,0.05,2.4);
	glutSolidCube(3.0f);
    glPopMatrix();

    glPushMatrix();
	glTranslatef(-3.5f, 29.7f, -0.0f);
	glColor4f(0.8, 0.8, 0.8, 0.8);
	glScalef(0.05,0.05,2.4);
	glutSolidCube(3.0f);
    glPopMatrix();

    glPushMatrix();
	glTranslatef(-3.5f, 29.0f, -1.25f);
	glColor4f(0.8, 0.8, 0.8, 0.8);
	glScalef(0.05,0.5,0.05);
	glutSolidCube(3.0f);
    glPopMatrix();
}

void atap()
{
    glPushMatrix();
	glTranslatef(0.0f, 32.5f, 0.0f);
	//glRotatef(-60.0f,1.0f,10.0f,10.0f);
	glColor4f(0.8, 0.8, 0.8, 0.8);
	//glScalef(1.0,1.0,3.0);
	glBegin(GL_QUADS);
        glVertex3f(-3.5f, -1.0f, -3.5f);
        glVertex3f(0.0f, 6.0f, -3.3f);
        glVertex3f(0.0f, 1.8f, -2.3f);
        glVertex3f(0.0f, 1.0f, 0.0f);

        glVertex3f(3.5f, -1.0f, -3.5f);
        glVertex3f(0.0f, 6.0f, -3.3f);
        glVertex3f(0.0f, 1.8f, -2.3f);
        glVertex3f(0.0f, 1.0f, 0.0f);

        glVertex3f(-3.5f, -1.0f, 3.5f);
        glVertex3f(0.0f, 6.0f, 3.3f);
        glVertex3f(0.0f, 1.8f, 2.3f);
        glVertex3f(0.0f, 1.0f, 0.0f);

        glVertex3f(3.5f, -1.0f, 3.5f);
        glVertex3f(0.0f, 6.0f, 3.3f);
        glVertex3f(0.0f, 1.8f, 2.3f);
        glVertex3f(0.0f, 1.0f, 0.0f);

        glVertex3f(3.5f, -1.0f, -3.5f);
        glVertex3f(3.3f, 6.0f, 0.0f);
        glVertex3f(2.3f, 1.8f, 0.0f);
        glVertex3f(0.0f, 1.0f, 0.0f);

        glVertex3f(3.5f, -1.0f, 3.5f);
        glVertex3f(3.3f, 6.0f, 0.0f);
        glVertex3f(2.3f, 1.8f, 0.0f);
        glVertex3f(0.0f, 1.0f, 0.0f);

        glVertex3f(-3.5f, -1.0f, 3.5f);
        glVertex3f(-3.3f, 6.0f, 0.0f);
        glVertex3f(-2.3f, 1.8f, 0.0f);
        glVertex3f(0.0f, 1.0f, 0.0f);

        glVertex3f(-3.5f, -1.0f, -3.5f);
        glVertex3f(-3.3f, 6.0f, 0.0f);
        glVertex3f(-2.3f, 1.8f, 0.0f);
        glVertex3f(0.0f, 1.0f, 0.0f);
    glEnd();
    glPopMatrix();
}
//===========================================BATAS JAM GADANG=====================================================//

//POHON
void pohon()
{
    //Batang Cemara
    glPushMatrix();
	glColor3f(0.8f, 0.4f, 0.1f);
    glScaled(1.1,7,1);
    glutSolidCube(0.5f);
    glPopMatrix();

    //Daun Bawah
    glPushMatrix();
    glColor3f(0.5f, 0.7f, 0.1f);
    glTranslatef(0.0f, 1.5f, 0.0f);
    glRotatef(230, 1.5, 2, 2);
	glScaled(2,2,3);
	glutSolidCone(1.6,1,20,30);
	glPopMatrix();

    //Daun Tengah
    glPushMatrix();
    glColor3f(0.5f, 0.7f, 0.1f);
    glTranslatef(0.0f, 3.0f, 0.0f);
    glRotatef(230, 1.5, 2, 2);
	glScaled(2,2,3);
	glutSolidCone(1.3,1,20,30);
	glPopMatrix();

	//Daun Atas
    glPushMatrix();
    glColor3f(0.5f, 0.7f, 0.1f);
    glTranslatef(0.0f, 4.5f, 0.0f);
    glRotatef(230, 1.5, 2, 2);
	glScaled(2,2,3);
	glutSolidCone(1.0,1,20,30);
	glPopMatrix();

}

//POHON TAMAN
void pohontaman()
{
    //Batang Pohon Tengah
    glPushMatrix();
	glColor3f(0.5f, 0.5f, 0.5f);
	glTranslatef(0.0, 1.9, 0.0);
    glScaled(0.3 , 7.5, 0.3);
    glutSolidCube(0.5f);
    glPopMatrix();
    //Lantai
    glPushMatrix();
    glColor3f(1.0, 1.0, 1.0);
    //glTranslatef(0.0, 3.6, 0.0);
    glScaled(0.7, 0.09, 0.7);
    glutSolidCube(0.5f);
    glPopMatrix();
    //Batang Pohon Kiri
    glPushMatrix();
	glColor3f(0.0f, 1.0f, 0.0f);
	glTranslatef(0.0, 3.6, 0.0);
	glRotatef(45, 0.0, 0.0, 2.0);
    glScaled(0.15 ,5 ,0.15);
    glutSolidCube(0.5f);
    glPopMatrix();

    //Batang Pohon Kanan
    glPushMatrix();
	glColor3f(0.8f, 0.8f, 0.0f);
	glTranslatef(0.0, 3.6, 0.0);
	glRotatef(45, 0.0, 0.0, -2.0);
    glScaled(0.15 ,5 ,0.15);
    glutSolidCube(0.5f);
    glPopMatrix();

    //Batang Pohon Depan
    glPushMatrix();
	glColor3f(1.0f, 0.0f, 0.0f);
	glTranslatef(0.0, 3.6, 0.0);
	glRotatef(45, 2.0, 0.0, 0.0);
    glScaled(0.15 ,5 ,0.15);
    glutSolidCube(0.5f);
    glPopMatrix();

    //Batang Pohon Belakang
    glPushMatrix();
	glColor3f(0.0f, 0.0f, 1.0f);
	glTranslatef(0.0, 3.6, 0.0);
	glRotatef(45, -2.0, 0.0, 0.0);
    glScaled(0.15 ,5 ,0.15);
    glutSolidCube(0.5f);
    glPopMatrix();

    //Batang Pohon Lurus
    glPushMatrix();
	glColor3f(0.0f, 0.4f, 0.4f);
	glTranslatef(0.0, 3.6, 0.0);
	glRotatef(90, -2.0, 0.0, 0.0);
    glScaled(0.15 ,5 ,0.15);
    glutSolidCube(0.5f);
    glPopMatrix();

    //Batang Pohon Lurus
    glPushMatrix();
	glColor3f(0.8f, 0.0f, 0.8f);
	glTranslatef(0.0, 3.6, 0.0);
	glRotatef(90, 0.0, 0.0, 2.0);
    glScaled(0.15 ,5 ,0.15);
    glutSolidCube(0.5f);
    glPopMatrix();
}

//BANGKU TAMAN
void bangkutaman1()
{
    //bangku taman depan
    glPushMatrix();
    glTranslatef(-11.5, 0.0, 2.0);
    glColor3f(0.8f, 0.4f, 0.1f);
    glScalef(7.3, 1.5, 2.3);
    glutSolidCube(1.0f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-11.5, 1.0, 3.2);
    glColor3f(0.8f, 0.4f, 0.1f);
    glScalef(7.3, 1.5, 0.3);
    glutSolidCube(1.0f);
    glPopMatrix();
}

void bangkutaman2()
{
    //bangku taman belakang
    glPushMatrix();
    glTranslatef(-11.5, 0.0, -2.0);
    glColor3f(0.8f, 0.4f, 0.1f);
    glScalef(7.3, 1.5, 2.3);
    glutSolidCube(1.0f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-11.5, 1.0, -3.2);
    glColor3f(0.8f, 0.4f, 0.1f);
    //glRotatef(-8.5f, 1.0f, 0.0f, 0.0f);
    glScalef(7.3, 1.5, 0.3);
    glutSolidCube(1.0f);
    glPopMatrix();
}

void bangkutaman3()
{
    //bangku taman belakang
    glPushMatrix();
    glTranslatef(-11.5, 0.0, -0.0);
    glColor3f(0.8f, 0.4f, 0.1f);
    glScalef(2.3, 1.5, 7.3);
    glutSolidCube(1.0f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-10.5, 1.0, -0.0);
    glColor3f(0.8f, 0.4f, 0.1f);
    glScalef(0.3, 1.5, 7.3);
    glutSolidCube(1.0f);
    glPopMatrix();
}

void bangkutaman4()
{
    //bangku taman belakang
    glPushMatrix();
    glTranslatef(-11.5, 0.0, -0.0);
    glColor3f(0.8f, 0.4f, 0.1f);
    glScalef(2.3, 1.5, 7.3);
    glutSolidCube(1.0f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-12.5, 1.0, -0.0);
    glColor3f(0.8f, 0.4f, 0.1f);
    glScalef(0.3, 1.5, 7.3);
    glutSolidCube(1.0f);
    glPopMatrix();
}

//JALUR TAMAN
void jalur()
{
     //Dasar
    glPushMatrix();
	glColor4f(0.5f, 0.5f, 0.5f, 1.0f);
	glTranslatef(-0.9, 0.0, 0.0);
	glScalef(3.0,0.02,4.5);
	glutSolidCube(9.0f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(20.4, .0, -35.0);
    //Dasar2
    glPushMatrix();
    glColor3f(0.0, 0.0, 0.0);
    glTranslatef(0.0, 0.0, 28.5);
    glScalef(1.7, 0.02, 1.9);
	glutSolidCube(9.0f);
    glPopMatrix();

    //Trotoar kanan
    glPushMatrix();
    glColor3f(0.5, 0.5, 0.5);
    glTranslatef(0.0, 0.0, 20.3);
    glScalef(1.6,0.1, 0.05);
	glutSolidCube(9.0f);
    glPopMatrix();

    //Trotoar Kiri
    glPushMatrix();
    glColor3f(0.5, 0.5, 0.5);
    glTranslatef(0.0, 0.0, 36.7);
    glScalef(1.6,0.1, 0.05);
	glutSolidCube(9.0f);
    glPopMatrix();

    glPopMatrix();

}

//KINCIR
void kincir()
{   /*//Lantai
    glPushMatrix();
    glColor4f(0.8, 0.8, 0.8, 0.8);
    glTranslatef(0.0, -15.0, 0.0);
	glScalef(4.0, 1.09, 2.5);
	glutSolidCube(6.0f);
    glPopMatrix();*/

    //Atap1
    glPushMatrix();
    glColor4f(0.8, 0.8, 0.8, 0.8);
    glTranslatef(0.0, 0.0, 0.0);
	glScalef(3.0,0.09,1.5);
	glutSolidCube(6.0f);
    glPopMatrix();

    //Atap2
    glPushMatrix();
    glColor4f(0.8, 0.8, 0.8, 0.8);
    glTranslatef(0.0, -7.0, 0.0);
	glScalef(3.0,0.09,1.5);
	glutSolidCube(6.0f);
    glPopMatrix();

    //Atap3
    glPushMatrix();
    glColor4f(0.8, 0.8, 0.8, 0.8);
    glTranslatef(0.0, 7.0, 0.0);
	glScalef(3.0,0.09,1.5);
	glutSolidCube(6.0f);
    glPopMatrix();

    //Atap4
    glPushMatrix();
    glColor4f(0.8, 0.8, 0.8, 0.8);
    glTranslatef(0.0, 14.0, 0.0);
	glScalef(3.0,0.09,1.5);
	glutSolidCube(6.0f);
    glPopMatrix();

    //Atap5
    glPushMatrix();
    glColor4f(0.8, 0.8, 0.8, 0.8);
    glTranslatef(0.0, 21.0, 0.0);
	glScalef(3.0,0.09,1.5);
	glutSolidCube(6.0f);
    glPopMatrix();

    //Generator
    glPushMatrix();
    glColor4f(0.8, 0.8, 0.8, 0.8);
    glTranslatef(0.0, 22.0, 0.0);
	glScalef(2.0,0.59,1.0);
	glutSolidCube(6.0f);
    glPopMatrix();
    /*//Listrik Bawah
    glPushMatrix();
    glColor4f(1.8, 0.8, 0.8, 0.8);
    glTranslatef(-1.0, 26.0, 0.0);
	glScalef(0.1,0.1,0.1);
	glutSolidSphere(5, 20, 30);
    glPopMatrix();
    //Listrik Tengah
    glPushMatrix();
    glColor4f(1.8, 0.8, 0.8, 0.8);
    glTranslatef(-1.0, 27.0, 0.0);
	glScalef(0.1,0.1,0.1);
	glutSolidSphere(5, 20, 30);
    glPopMatrix();
    //Listrik Atas
    glPushMatrix();
    glColor4f(1.8, 0.8, 0.8, 0.8);
    glTranslatef(-1.0, 28.0, 0.0);
	glScalef(0.1,0.1,0.1);
	glutSolidSphere(5, 20, 30);
    glPopMatrix();
    */

    //Pemutar Depan
    glPushMatrix();
    glColor4f(0.8, 0.8, 0.8, 0.8);
    glTranslatef(0.0, 23.0, 4.0);
	glScalef(0.2,0.09,1.0);
	glutSolidCube(6.0f);
    glPopMatrix();
    //Bulatan
    glPushMatrix();
    glColor4f(0.8, 0.8, 0.8, 0.8);
    glTranslatef(0.0, 23.0, 6.5);
	glScalef(0.2,0.2,0.2);
	glutSolidSphere(5, 20, 30);
    glPopMatrix();
    //Pemutar Belakang
    glPushMatrix();
    glColor4f(0.8, 0.8, 0.8, 0.8);
    glTranslatef(0.0, 22.0, -10.0);
	glScalef(0.2,0.59,0.5);
	glutSolidCube(6.0f);
    glPopMatrix();

    glPushMatrix();
    glColor4f(0.8, 0.8, 0.8, 0.8);
    glTranslatef(0.0, 22.0, -5.0);
	glScalef(0.2,0.19,1.7);
	glutSolidCube(6.0f);
    glPopMatrix();

    glPushMatrix();
    glColor4f(1.0, 1.8, 0.8, 0.8);
    glTranslatef(0.0, 22.0, -10.0);
    glScalef(0.8, 0.1, 0.5);
    glutSolidCube(6.0f);
    glPopMatrix();

    //Tiang Kiri Depan
    glPushMatrix();
    glColor4f(0.8, 0.8, 0.8, 0.8);
    glTranslatef(-7.5, 0.0, 2.5);
	glScalef(0.15,7.0,0.25);
	glutSolidCube(6.0f);
    glPopMatrix();

    //Tiang Kiri Belakang
    glPushMatrix();
    glColor4f(0.8, 0.8, 0.8, 0.8);
    glTranslatef(-7.5, 0.0, -2.5);
	glScalef(0.15,7.0,0.25);
	glutSolidCube(6.0f);
    glPopMatrix();

    //Tiang Kanan Depan
    glPushMatrix();
    glColor4f(0.8, 0.8, 0.8, 0.8);
    glTranslatef(7.5, 0.0, 2.5);
	glScalef(0.15,7.0,0.25);
	glutSolidCube(6.0f);
    glPopMatrix();

    //Tiang Kanan Belakang
    glPushMatrix();
    glColor4f(0.8, 0.8, 0.8, 0.8);
    glTranslatef(7.5, 0.0, -2.5);
	glScalef(0.15,7.0,0.25);
	glutSolidCube(6.0f);
    glPopMatrix();

    //Kipas
    glPushMatrix();
    glTranslatef(0.0, 23.0, 6.5);
    glColor4f(0.0, 0.0, 0.0, 1.0);
    glScalef(2.15, 2.0, 0.55);
	glRotatef(sudutk, 0.0f, 0.0f, 1.0f);
    glutWireTorus(1, 5, 10, 15);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0, 23.0, 6.5);
    glColor4f(0.0, 0.0, 0.0, 1.0);
    glScalef(1.15, 1.0, 0.55);
	glRotatef(sudutk, 0.0f, 0.0f, 1.0f);
    glutWireTorus(1, 5, 10, 15);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0, 23.0, 6.5);
    glColor4f(0.0, 0.0, 0.0, 1.0);
    glScalef(0.55, 0.5, 0.55);
	glRotatef(sudutk, 0.0f, 0.0f, 1.0f);
    glutWireTorus(1, 5, 10, 15);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0, 23.0, 6.5);
    glColor4f(0.0, 0.0, 0.0, 1.0);
    glScalef(3.15, 3.0, 0.55);
	glRotatef(sudutk, 0.0f, 0.0f, 1.0f);
    glutWireTorus(1, 5, 10, 15);
    glPopMatrix();

}

//PUTAR KIPAS
void putar(int value)
{
    sudutk += 5.0f;
    if (sudutk > 360){
        sudutk -= 360;
    }

    glutPostRedisplay();
    glutTimerFunc(25, putar, 0);
}

//BUS
void bus()
{
    //Bodi
    glColor3f(1.0, 0.8, 0.0);
    glPushMatrix();
    //glRotatef(sudutk, 0.0, 0.0, 1.0);
    glTranslatef(0.0, 3.8, 0.0);
    glScalef(4.0, 1.0, 1.5);
    glutSolidCube(6.0f);
    glPopMatrix();

    //Kaca Belakang
    glColor3f(0.0, 0.0, 1.0);
    glPushMatrix();
    glTranslatef(-12.0, 5.0, 0.0);
    glScalef(0.05, 0.5, 1.2);
    glutSolidCube(6.0f);
    glPopMatrix();

    //Lampu Rem
    glColor3f(1.0, 0.0, 0.0);
    glPushMatrix();
    glTranslatef(-12.1, 2.0, -3.5);
    glScalef(0.02, 0.19, 0.08);
    glutSolidCube(6.0f);
    glPopMatrix();

    glColor3f(1.0, 0.0, 0.0);
    glPushMatrix();
    glTranslatef(-12.1, 2.0, 3.5);
    glScalef(0.02, 0.19, 0.08);
    glutSolidCube(6.0f);
    glPopMatrix();

    //Lampu Depan
    glColor3f(1.0, 1.0, 1.0);
    glPushMatrix();
    glTranslatef(12.1, 2.0, -3.5);
    glScalef(0.05, 0.02, 0.1);
    glutSolidCube(6.0f);
    glPopMatrix();

    glColor3f(1.0, 1.0, 1.0);
    glPushMatrix();
    glTranslatef(12.1, 2.0, 3.5);
    glScalef(0.05, 0.02, 0.1);
    glutSolidCube(6.0f);
    glPopMatrix();

    //Kaca Depan
    glColor3f(0.0, 0.0, 1.0);
    glPushMatrix();
    glTranslatef(12.0, 5.0, 0.0);
    glScalef(0.05, 0.5, 1.2);
    glutSolidCube(6.0f);
    glPopMatrix();
    //Kaca pinggir
    glColor3f(0.0, 0.0, 1.0);
    glPushMatrix();
    glTranslatef(-1.5, 5.0, 4.0);
    glScalef(3.3, 0.5, 0.2);
    glutSolidCube(6.0f);
    glPopMatrix();

    glColor3f(0.0, 0.0, 1.0);
    glPushMatrix();
    glTranslatef(-1.5, 5.0, -4.0);
    glScalef(3.3, 0.5, 0.2);
    glutSolidCube(6.0f);
    glPopMatrix();
    //Pintu Kanan
    glColor3f(0.5, 0.5, 0.5);
    glPushMatrix();
    glTranslatef(11.1, 3.9, 4.0);
    glScalef(0.15, 0.8, 0.2);
    glutSolidCube(6.0f);
    glPopMatrix();

    glPushMatrix();
    glColor3f(0.5, 0.5, 0.5);
    glTranslatef(10.1, 5.1, 4.0);
    glScalef(0.25, 0.4, 0.2);
    glutSolidCube(6.0f);
    glPopMatrix();
    //Pintu Kiri
    glColor3f(0.5, 0.5, 0.5);
    glPushMatrix();
    glTranslatef(11.1, 3.9, -4.0);
    glScalef(0.15, 0.8, 0.2);
    glutSolidCube(6.0f);
    glPopMatrix();

    glPushMatrix();
    glColor3f(0.5, 0.5, 0.5);
    glTranslatef(10.1, 5.1, -4.0);
    glScalef(0.25, 0.4, 0.2);
    glutSolidCube(6.0f);
    glPopMatrix();

    //Ban Belakang
    glPushMatrix();
    glColor3f(1.0, 1.0, 1.0);
    glScalef(0.35, 0.35, 0.25);
    glTranslatef(25.0, 4.5, 19.3);
    glutSolidTorus(2, 3, 20, 30);
    glPopMatrix();
    //Velg
    glPushMatrix();
    glColor3f(0.0, 0.0, 0.0);
    glScalef(0.5, 0.5, 0.5);
    glTranslatef(17.5, 2.5, 8.0);
    glutSolidSphere(2, 10, 20);
    glPopMatrix();

    glPushMatrix();
    glColor3f(1.0, 1.0, 1.0);
    glScalef(0.35, 0.35, 0.25);
    glTranslatef(25.0, 4.5, -19.3);
    glutSolidTorus(2, 3, 20, 30);
    glPopMatrix();
    //Velg
    glPushMatrix();
    glColor3f(0.0, 0.0, 0.0);
    glScalef(0.5, 0.5, 0.5);
    glTranslatef(-17.5, 2.5, 8.0);
    glutSolidSphere(2, 10, 20);
    glPopMatrix();


    //Ban Depan
    glPushMatrix();
    glColor3f(1.0, 1.0, 1.0);
    glScalef(0.35, 0.35, 0.25);
    glTranslatef(-25.0, 4.5, -19.3);
    glutSolidTorus(2, 3, 20, 30);
    glPopMatrix();
    //Velg
    glPushMatrix();
    glColor3f(0.0, 0.0, 0.0);
    glScalef(0.5, 0.5, 0.5);
    glTranslatef(17.5, 2.5, -8.0);
    glutSolidSphere(2, 10, 20);
    glPopMatrix();

    glPushMatrix();
    glColor3f(1.0, 1.0, 1.0);
    glScalef(0.35, 0.35, 0.25);
    glTranslatef(-25.0, 4.5, 19.3);
    glutSolidTorus(2, 3, 20, 30);
    glPopMatrix();
    //Velg
    glPushMatrix();
    glColor3f(0.0, 0.0, 0.0);
    glScalef(0.5, 0.5, 0.5);
    glTranslatef(-17.5, 2.5, -8.0);
    glutSolidSphere(2, 10, 20);
    glPopMatrix();
}

unsigned int LoadTextureFromBmpFile(char *filename);

void display(void) {
	glClearStencil(0); //clear the stencil buffer
	glClearDepth(1.0f);
	glClearColor(0.0, 0.6, 0.8, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); //clear the buffers
	glLoadIdentity();
	gluLookAt(viewx-110, viewy+20, viewz+20, 0.0, 10.0, 5.0, 0.0, 1.0, 0.0);

	glPushMatrix();

	//glBindTexture(GL_TEXTURE_3D, texture[0]);
	drawSceneTanah(_terrain, 0.0f, 0.5f, 0.0f);
	glPopMatrix();

	glPushMatrix();

	//glBindTexture(GL_TEXTURE_3D, texture[0]);
	drawSceneTanah(_terrainTanah, 0.0f, 0.0f, 0.0f);
	glPopMatrix();

	glPushMatrix();

	//glBindTexture(GL_TEXTURE_3D, texture[0]);
	drawSceneTanah(_terrainAir, 0.0f, 0.2f, 0.5f);
	glPopMatrix();

//=========================================================================================//
//====================================MULAI OBJEK===========================================//
//=========================================================================================//

//LAMPU
    //kiri depan
    glPushMatrix();
    glTranslatef(0.3,0.05,-5.0);
    lampu();
    glPopMatrix();
    //kiri belakang
    glPushMatrix();
    glTranslatef(0.3,0.05,-45.0);
    lampu();
    glPopMatrix();
    //kanan depan
    glPushMatrix();
    glTranslatef(35.8,0.05,-5.0);
    lampu();
    glPopMatrix();
    //kanan belakang
    glPushMatrix();
    glTranslatef(35.8,0.05,-45.0);
    lampu();
    glPopMatrix();

//LAMPU JALAN
    for (int i = -2; i < 3; i++)
        {
            glPushMatrix();
            glTranslatef(i*35, 5, 38);
            lampuJalan();
            glPopMatrix();
        }

    for (int i = -2; i < 3; i++)
        {
            glPushMatrix();
            glTranslatef(i*35, 5, 11);
            lampuJalan2();
            glPopMatrix();
        }

//BANGUNAN
    glPushMatrix();
    glTranslatef(18.0,-0.5,-25.0);
    bangunan();
    glPopMatrix();

//JALUR TAMAN
    glPushMatrix();
    glTranslatef(-28.0,-0.5,-25.0);
    jalur();
    glPopMatrix();

//JAM GADANG
    glPushMatrix();
    glTranslatef(18.0, 9.0,-35.0);
    glScaled(1.7, 1.7, 1.7);
    lantai1();
    jendelalantai1();
    tangga();
    pagartangga();
    lantai2();
    jendelalantai2();
    pagarlantai2();
    lantai3();
    jendelalantai3();
    lantai4();
    jendelalantai4();
    lantai5();
    jam();
    lantaiatas();
    atap();
    glPopMatrix();

//POHON
for (int i = -5; i < 5; i++)
        {
            glPushMatrix();
            glTranslatef(i*15, 1, 54);
            pohon();
            glPopMatrix();
        }
    glPushMatrix();
    glTranslatef(75.0,1,54.0);
    pohon();
    glPopMatrix();

    //Pohon belakang
    glPushMatrix();
    glTranslatef(65.0,7,-35.0);
    pohon();
    glPopMatrix();

    //Pohon Pinggir
    glPushMatrix();
    glTranslatef(-50, 1.2, -45);
    pohon();
    glPopMatrix();

    //Pohon Pinggir
    glPushMatrix();
    glTranslatef(-55, 1.2, -15);
    pohon();
    glPopMatrix();

    //Pohon Pinggir
    glPushMatrix();
    glTranslatef(-62, 1.2, 1.7);
    pohon();
    glPopMatrix();


    //Pohon Taman Depan
    glPushMatrix();
    glColor4f(0.0f, 0.8f, 0.0f, 1.0f);
    glScalef(3.0, 3.0, 3.0);
    glTranslatef(-5.5, -0.1, -1.7);
    pohontaman();
    glPopMatrix();

    //Pohon Taman Depan
    glPushMatrix();
    glColor4f(0.0f, 0.8f, 0.0f, 1.0f);
    glScalef(3.0, 3.0, 3.0);
    glTranslatef(-13.9, -0.1, -1.7);
    pohontaman();
    glPopMatrix();

    //Pohon Taman Belakang
    glPushMatrix();
    glColor4f(0.0f, 0.8f, 0.0f, 1.0f);
    glScalef(3.0, 3.0, 3.0);
    glTranslatef(-5.5, -0.1, -14.9);
    pohontaman();
    glPopMatrix();

    //Pohon Taman Belakang
    glPushMatrix();
    glColor4f(0.0f, 0.8f, 0.0f, 1.0f);
    glScalef(3.0, 3.0, 3.0);
    glTranslatef(-13.9, -0.1, -14.9);
    pohontaman();
    glPopMatrix();

    //Pohon Taman Tengah
    glPushMatrix();
    glColor4f(0.0f, 0.8f, 0.0f, 1.0f);
    glScalef(3.0, 3.0, 3.0);
    glTranslatef(-9.5, -0.1, -8.9);
    pohontaman();
    glPopMatrix();

//Bangku Taman depan kiri
   glPushMatrix();
   glScalef(1.0f,1.0f,1.0f);
   glTranslatef(-27.0,-0.1,-8.9);
   bangkutaman1();
   glPopMatrix();

//Bangku Taman depan kanan
   glPushMatrix();
   glScalef(1.0f,1.0f,1.0f);
   glTranslatef(-8.0,-0.1,-8.9);
   bangkutaman1();
   glPopMatrix();

//Bangku Taman belakang kiri
   glPushMatrix();
   glScalef(1.0f,1.0f,1.0f);
   glTranslatef(-27.0,-0.1,-41.0);
   bangkutaman2();
   glPopMatrix();

//Bangku Taman belakang kanan
   glPushMatrix();
   glScalef(1.0f,1.0f,1.0f);
   glTranslatef(-8.0,-0.1,-41.0);
   bangkutaman2();
   glPopMatrix();

//Bangku Taman kanan belakang
   glPushMatrix();
   glScalef(1.0f,1.0f,1.0f);
   glTranslatef(-5.5,-0.1,-40.5);
   bangkutaman3();
   glPopMatrix();

//Bangku Taman kanan depan
   glPushMatrix();
   glScalef(1.0f,1.0f,1.0f);
   glTranslatef(-5.5,-0.1,-9.2);
   bangkutaman3();
   glPopMatrix();

//Bangku Taman kiri depan
   glPushMatrix();
   glScalef(1.0f,1.0f,1.0f);
   glTranslatef(-29.5,-0.1,-9.2);
   bangkutaman4();
   glPopMatrix();

//Bangku Taman kiri belakang
   glPushMatrix();
   glScalef(1.0f,1.0f,1.0f);
   glTranslatef(-29.5,-0.1,-40.5);
   bangkutaman4();
   glPopMatrix();


//MARKA JALAN
    for (int i = -3; i < 4; i++)
        {
            glPushMatrix();
            glTranslatef(i*22, 0.0, 25);
            marka();
            glPopMatrix();
        }

//PAGAR
        glPushMatrix();
        glTranslatef(27, 0.0, 42);
        pagar();
        glPopMatrix();

//BUS
    glPushMatrix();
    glTranslatef(a-27, b-0.0, c+18);
    bus();
    glPopMatrix();


//KINCIR
glPushMatrix();
glScalef(0.5, 0.5, 0.5);
glTranslatef(124.0, 20.0,-63.0);
glRotatef(45, 0.0, -1.0, 0.0);
        glPushMatrix();
        glTranslatef(-20.0,-0.5,-25.0);
        kincir();
        glPopMatrix();
glPopMatrix();

	glutSwapBuffers();
	glFlush();
	rot++;
	angle++;

}

void init(void) {
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glDepthFunc(GL_LESS);
	glEnable(GL_NORMALIZE);
	glEnable(GL_COLOR_MATERIAL);
	glDepthFunc(GL_LEQUAL);
	glShadeModel(GL_SMOOTH);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glEnable(GL_CULL_FACE);

	_terrain = loadTerrain("Terrain.bmp", 20);
	_terrainTanah = loadTerrain("Jalan.bmp", 20);
	_terrainAir = loadTerrain("heightmapAir.bmp", 20);

	//binding texture

}

static void kibor(int key, int x, int y) {
	switch (key) {
	case GLUT_KEY_HOME:
		viewy++;
		break;
	case GLUT_KEY_END:
		viewy--;
		break;
	case GLUT_KEY_UP:
		viewz--;
		break;
	case GLUT_KEY_DOWN:
		viewz++;
		break;

	case GLUT_KEY_RIGHT:
		viewx++;
		break;
	case GLUT_KEY_LEFT:
		viewx--;
		break;

	/*case GLUT_KEY_F1: {
		glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
		glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	}
		;
		break;
	case GLUT_KEY_F2: {
		glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient2);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse2);
		glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	}
		;
		break;*/
	default:
		break;
	}
}

void newkeyboard(unsigned char key, int x, int y) {
	/*if (key == 'd') {

		spin = spin - 1;
		if (spin > 360.0)
			spin = spin - 360.0;
	}
	if (key == 'a') {
		spin = spin + 1;
		if (spin > 360.0)
			spin = spin - 360.0;
	}*/
	if (key == 'q') {
		viewz++;
	}
	if (key == 'e') {
		viewz--;
	}
	if (key == 's') {
		viewy--;
	}
	if (key == 'w') {
		viewy++;
	}
//UNTUK BUS
    if  (key == 'm'){
        a+=-1;
    d+=1;
    b=0;
    c=0;
      }
      else if (key=='n'){
           d+=-1;
  a+=1;
  b=0;
  c=0;
    }
    if  (key == 27){ //ESC
        exit(0);
    }
}

void reshape(int w, int h) {
	glViewport(0, 0, (GLsizei) w, (GLsizei) h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60, (GLfloat) w / (GLfloat) h, 0.1, 1000.0);
	glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char **argv) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_STENCIL | GLUT_DEPTH); //add a stencil buffer to the window
	glutInitWindowSize(800, 600);
	glutInitWindowPosition(100, 100);
	glutCreateWindow("Tugas Besar Grafkom- Jam Gadang");
	init();
	initRendering();

	glutDisplayFunc(display);
	glutIdleFunc(display);
	glutReshapeFunc(reshape);
	glutSpecialFunc(kibor);

	glutKeyboardFunc(newkeyboard);

	glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);

	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, high_shininess);
	glColorMaterial(GL_FRONT, GL_DIFFUSE);

    glutTimerFunc(25, putar, 0);
	glutMainLoop();
	return 0;
}
