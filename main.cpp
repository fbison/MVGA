#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <list>
#include <cstdlib>
#include <stdio.h>
#include <iostream>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <time.h>
//#include <functional>

#include "of.h"
#include "ofOffPointsReader.h"
#include "Handler.hpp" 
#include "GL_Interactor.h"
#include "ColorRGBA.hpp"
#include "Cores.h"
#include "Point.hpp"
#include "printof.hpp"


#include "CommandComponent.hpp"
#include "MyCommands.hpp"

#include "ofVertexStarIteratorSurfaceVertex.h"


clock_t start_insert;
clock_t end_insert;
clock_t start_print;
clock_t end_print;

using namespace std;
using namespace of;

#define COORDENADAx 0
#define COORDENADAy 1
#define FRONTEIRA -1
#define NAUMENCONTRADO -2
#define ENCONTRADO -3
#define CELULA_INICIAL 255
//Define o tamanho da tela.
scrInteractor *Interactor = new scrInteractor(900, 650);

//Define a malha a ser usada.
typedef of::MyofDefault2D TTraits;
typedef of::ofMesh<TTraits> TMesh;
TMesh *malha;
Handler<TMesh> meshHandler;

typedef PrintOf<TTraits> TPrintOf;

TPrintOf *Print;

typedef MyCommands<TPrintOf> TMyCommands;
typedef CommandComponent TAllCommands;

ofVtkWriter<TTraits> writer;
TAllCommands *allCommands;

//##################################################################//

////////////////////////////////////////////////////////////////////////
int type = 3;
//CASO 1 EXECUTA CRUST
//CASO 2 EXECUTA BETA-SKELETON
//CASO 3 EXECUTA ARVORE
////////////////////////////////////////////////////////////////////////

class Escalares {
  public:
    double alpha;
    double beta;
    double delta;
};

class Coordenadas {
  public:
    double x;
    double y;
};

class Triangulo {
  public:
    Coordenadas a;
    Coordenadas b;
    Coordenadas c;
};
Triangulo newTriangulo(double ax, double ay,
						double bx, double by,
						double cx, double cy){
	Triangulo triangulo;
	triangulo.a.x = ax;
    triangulo.a.y = ay;
    triangulo.b.x = bx;
    triangulo.b.y = by;
    triangulo.c.x = cx;
    triangulo.c.y = cy;
	
	return triangulo;
}

void printTriangulo(Triangulo e){
    std::cout<<"Triangulo: ax" <<e.a.x<< " ay:" <<e.a.y<< " bx:" <<e.b.x<< " by:" <<e.b.y<< "  cx:" <<e.c.x<< " by:" <<e.c.y   <<std::endl;
}
double calculaArea(Triangulo t){
	double area=(
		0.5*
		((t.a.x*t.b.y)-(t.a.y*t.b.x)+
		 (t.a.y*t.c.x)-(t.a.x*t.c.y)+
		 (t.b.x*t.c.y)-(t.b.y*t.c.x))
	);
	std::cout<<"Area Triangulo: " <<area <<std::endl;

	return area; 
}
Triangulo obtemCoordenadas(int id){
	Triangulo triangulo;
    std::cout<<"Triangulo: id" <<id <<std::endl;
	triangulo.a.x = malha->getVertex(malha->getCell(id)->getVertexId(0))->getCoord(COORDENADAx);
    triangulo.a.y = malha->getVertex(malha->getCell(id)->getVertexId(0))->getCoord(COORDENADAy);
    triangulo.b.x = malha->getVertex(malha->getCell(id)->getVertexId(1))->getCoord(COORDENADAx);
    triangulo.b.y = malha->getVertex(malha->getCell(id)->getVertexId(1))->getCoord(COORDENADAy);
    triangulo.c.x = malha->getVertex(malha->getCell(id)->getVertexId(2))->getCoord(COORDENADAx);
    triangulo.c.y = malha->getVertex(malha->getCell(id)->getVertexId(2))->getCoord(COORDENADAy);
    printTriangulo(triangulo);
	return triangulo;
}
void printBaricentrica(Escalares e){
    std::cout<<"Baricentrica: alpha" <<e.alpha<< " beta:" <<e.beta << "delta:" <<e.delta  <<std::endl;
}
//calculo do sistema passado em aula
Escalares calculaBaricentrica(Coordenadas pontoP, int id){
    std::cout<<"//////////////// calculaBaricentrica: id" <<id <<std::endl;
	Triangulo triangulo = obtemCoordenadas(id);
	Triangulo trianguloCopia = triangulo;
    std::cout<<"calculaArea: ABC" <<std::endl;
    printTriangulo(triangulo);
	double areaABC= calculaArea(triangulo);
	triangulo.c = pontoP;
    std::cout<<"calculaArea: ABP" <<std::endl;
    printTriangulo(triangulo);
	double areaABP= calculaArea(triangulo);
	triangulo.c = trianguloCopia.c;	
	triangulo.a = pontoP;
    std::cout<<"calculaArea: PBC" <<std::endl;
    printTriangulo(triangulo);
	double areaPBC= calculaArea(triangulo);
	triangulo.a = trianguloCopia.a;	
	triangulo.b = pontoP;
    std::cout<<"calculaArea: APC" <<std::endl;
    printTriangulo(triangulo);
	double areaAPC= calculaArea(triangulo);
	Escalares escalares;
	escalares.alpha= areaPBC/areaABC;
	escalares.beta= areaAPC/areaABC;
	escalares.delta= areaABP/areaABC;
    printBaricentrica(escalares);
    std::cout<<"//////////////// fim calc. baricentrica: id" <<id <<std::endl;
	return escalares;
}

int defineProximoTriangulo(Escalares baricentrica){
	if (baricentrica.alpha > 0 && baricentrica.beta > 0 && baricentrica.delta > 0){
        return ENCONTRADO;
    }
    if (baricentrica.alpha < baricentrica.beta && baricentrica.alpha < baricentrica.delta) {
			return  0;
	}
	if (baricentrica.beta < baricentrica.alpha &&
		 baricentrica.beta < baricentrica.delta){
		 	return 1;
	} 
	return 2;
}

void buscaPeloPontoP(Coordenadas pontoP, int id){
	int proximo= NAUMENCONTRADO;
	while(proximo!= ENCONTRADO){
		std::cout<<"Passou por: " <<id <<std::endl;
		Escalares baricentrica = calculaBaricentrica(pontoP, id);
		proximo= defineProximoTriangulo(baricentrica);
		if(proximo == ENCONTRADO){
			std::cout<<"Encontrado: " <<id <<std::endl;
			Print->Face(malha->getCell(id), red);
			break;
		}else if(id!= CELULA_INICIAL){
			std::cout<<"Coloriu de verde por: " <<id <<std::endl;
			Print->Face(malha->getCell(id), dgreen);
		}
		id= malha->getCell(id)->getMateId(proximo);
		std::cout<<"Próximo triangulo: " <<id <<std::endl;
		if(id == FRONTEIRA){
			std::cout<<"Fronteira: " <<id <<std::endl;
			//validação se chegou na fronteira
			break;
		}
	}
	
}

Coordenadas coordenadaSelecionada(){
     Coordenadas p;
	 p.x = Interactor->getCoordenadaXClicada(); //coordenada x do clique direito
     p.y = Interactor->getCoordenadaYClicada(); //coordenada y do clique direito
	 std::cout<<"Cordenada: " <<p.x<< " " <<p.y <<std::endl;
	 return p;
}
void RenderScene(void){ 
	allCommands->Execute();
	Print->Vertices(malha,blue,3);
	Print->FacesWireframe(malha,grey,3);
	Print->Face(malha->getCell(CELULA_INICIAL), dgreen);
	//Como se fosse um useState do react, toda vez ele roda
	//e se detectar o clique faz o código
	Coordenadas pontoP;
	bool chave= false;
	pontoP= coordenadaSelecionada();
	std::cout<<"Cordenada: " <<pontoP.x<< " " <<pontoP.y <<std::endl;
	if(pontoP.x!= 0 && pontoP.y != 0){
		buscaPeloPontoP(pontoP, CELULA_INICIAL);
		glutPostRedisplay();
		glFinish();
		glutSwapBuffers();
	}
	glFinish();
	glutSwapBuffers();
}

void HandleKeyboard(unsigned char key, int x, int y){	
	double coords[3];
	char *xs[10];
	allCommands->Keyboard(key);
	
	switch (key) {

		case 'e':
			exit(1);
		break;
		case 'v':
			coords[0]=x;
			coords[1]=-y;
			coords[2]=0.0;
			malha->addVertex(coords);
		break;
		case 's':
			
			
		break;

		case 'd':
			
			
		break;
	

	}
    
	
	Interactor->Refresh_List();
	glutPostRedisplay();

}

using namespace std;

int main(int *argc, char **argv)
{

  ofRuppert2D<MyofDefault2D> ruppert;
  ofPoints2DReader<MyofDefault2D> reader;
  ofVtkWriter<MyofDefault2D> writer;
  Interactor->setDraw(RenderScene);
	meshHandler.Set(new TMesh());
      char *fileBrasil = "../Brasil.off";

     
    reader.readOffFile(fileBrasil);
    
    ruppert.execute2D(reader.getLv(),reader.getLids(),true);
    //writer.write(ruppert.getMesh(),"out.vtk",reader.getNorma(),ruppert.getNumberOfInsertedVertices());
  
  meshHandler = ruppert.getMesh();
  malha = ruppert.getMesh();
  
  
  Print = new TPrintOf(meshHandler);

	allCommands = new TMyCommands(Print, Interactor);

	double a,x1,x2,y1,y2,z1,z2; 

	of::ofVerticesIterator<TTraits> iv(&meshHandler);

	iv.initialize();
	x1 = x2 = iv->getCoord(0);
	y1 = y2 = iv->getCoord(1);
	z1 = z2 = iv->getCoord(2);

	for(iv.initialize(); iv.notFinish(); ++iv){
		if(iv->getCoord(0) < x1) x1 = a = iv->getCoord(0);
		if(iv->getCoord(0) > x2) x2 = a = iv->getCoord(0);
		if(iv->getCoord(1) < y1) y1 = a = iv->getCoord(1);
		if(iv->getCoord(1) > y2) y2 = a = iv->getCoord(1);
		if(iv->getCoord(2) < z1) z1 = a = iv->getCoord(2);
		if(iv->getCoord(2) > z2) z2 = a = iv->getCoord(2);
	}

	double maxdim;
	maxdim = fabs(x2 - x1);
	if(maxdim < fabs(y2 - y1)) maxdim = fabs(y2 - y1);
	if(maxdim < fabs(z2 - z1)) maxdim = fabs(z2 - z1);

	maxdim *= 0.6;
	
	Point center((x1+x2)/2.0, (y1+y2)/2.0, (y1+y2)/2.0 );
	Interactor->Init(center[0]-maxdim, center[0]+maxdim,
					center[1]-maxdim, center[1]+maxdim,
					center[2]-maxdim, center[2]+maxdim,argc,argv);

	
	
	AddKeyboard(HandleKeyboard);

	allCommands->Help(std::cout);
	std::cout<< std::endl<< "Press \"?\" key for help"<<std::endl<<std::endl;
	double t;
	
	Init_Interactor();

  
  return EXIT_SUCCESS;
}