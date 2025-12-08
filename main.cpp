// -------------------------------------------
// gMini : a minimal OpenGL/GLUT application
// for 3D graphics.
// Copyright (C) 2006-2008 Tamy Boubekeur
// All rights reserved.
// -------------------------------------------

// -------------------------------------------
// Disclaimer: this code is dirty in the
// meaning that there is no attention paid to
// proper class attribute access, memory
// management or optimisation of any kind. It
// is designed for quick-and-dirty testing
// purpose.
// -------------------------------------------

#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <utility>

#include <algorithm>
#include <GL/glew.h>
#include <GL/glut.h>
#include <float.h>

#include "src/util.h"

#include "src/Vec3.h"
#include "src/Camera.h"
#include "src/planet.h"
#include "src/tectonicPhenomenon.h"
#include "src/movement.h"
#include "src/erosion.h"
#include "src/rifting.h"
#include "src/amplification.h"
#include "src/ShaderProgram.h"
#include "src/Skybox.h"
#include "src/palette.h"



enum DisplayMode{ WIRE=0, SOLID=1, LIGHTED_WIRE=2, LIGHTED=3 };


// ------------------------------------
//Parametres Planete
// ------------------------------------

int nbPlates = 25;
int nbiter_resample = 15;
int spherepoints = 2048 * 24;


//Input mesh loaded at the launch of the application
Mesh mesh;

Planet planet(1.0f,spherepoints);
Movement movement_controller(planet);
Amplification* amplificator = nullptr;
int nbSteps = 0;
Erosion erosion_controller(planet);
float timeStep = 1.0f;
float elapsedSteps = 1.0f;

std::vector< float > current_field; //normalized filed of each vertex

bool display_normals;
bool display_smooth_normals;
bool display_mesh;
int display_plates_mode;
bool display_directions;

DisplayMode displayMode;
int weight_type;

static bool display_phenomena = false;
static bool display_atmosphere = true;

static Vec3 sunDirection = Vec3(22.0f, 16.0f, 50.0f);


// -------------------------------------------
// OpenGL/GLUT application code.
// -------------------------------------------

static GLint window;
static unsigned int SCREENWIDTH = 1600;
static unsigned int SCREENHEIGHT = 900;
static Camera camera;
static bool mouseRotatePressed = false;
static bool mouseMovePressed = false;
static bool mouseZoomPressed = false;
static int lastX=0, lastY=0, lastZoom=0;
static bool fullScreen = false;


// ------------------------------------
// variables globales shader
// ------------------------------------
float amplifiedPlanetRadius = 1.0f;
float planetRadiusMin = 1.0f;
float planetRadiusMax = 1.0f;

static ShaderProgram* atmosphereShader = nullptr;
static Skybox* starsSkybox = nullptr;
static GLuint atmosphereVAO = 0;
static GLuint atmosphereVBO = 0;
static GLuint atmosphereEBO = 0;
static int atmosphereIndexCount = 0;


void updateDisplayedColors() {
    planet.palette = Palette::getCurrentPalette();
    if (display_plates_mode == 0) {
        planet.colors = planet.vertexColorsForPlates();
        //printf("Updated colors for plates display.\n");
    } else if (display_plates_mode == 1) {
        planet.colors = planet.vertexColorsForCrustTypes();
        //printf("Updated colors for crust types display.\n");
    } else if (display_plates_mode == 2) {
        planet.colors = planet.vertexColorsForElevation();
        //printf("Updated colors for elevation display.\n");
    } else if (display_plates_mode == 3) {
        planet.colors = planet.vertexColorsForCrustTypesAmplified();
        planet.smoothColors();
        planet.smoothColors();
    }
    mesh.colors = planet.colors;
    glutPostRedisplay();
}

// ------------------------------------
// Application initialization
// ------------------------------------
void initLight () {
    // Normaliser la direction du soleil
    Vec3 sunDir = sunDirection;
    sunDir.normalize();
    
    // Position directionnelle (w=0 signifie lumière directionnelle à l'infini)
    GLfloat light_position1[4] = {-sunDir[0], -sunDir[1], -sunDir[2], 0.0f};
    GLfloat direction1[3] = {sunDir[0], sunDir[1], sunDir[2]};
    
    // Couleur du soleil (légèrement jaunâtre pour un effet réaliste)
    GLfloat color1[4] = {1.0f, 0.98f, 0.9f, 1.0f};
    GLfloat ambient[4] = {0.15f, 0.15f, 0.2f, 1.0f}; // Ambiance légèrement bleutée
    
    glLightfv(GL_LIGHT1, GL_POSITION, light_position1);
    glLightfv(GL_LIGHT1, GL_SPOT_DIRECTION, direction1);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, color1);
    glLightfv(GL_LIGHT1, GL_SPECULAR, color1);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);
    glEnable(GL_LIGHT1);
    glEnable(GL_LIGHTING);
}

void init() {

    camera.resize (SCREENWIDTH, SCREENHEIGHT);
    initLight ();
    glCullFace (GL_FRONT);
    glEnable(GL_CULL_FACE);
    glDepthFunc (GL_LESS);
    glEnable (GL_DEPTH_TEST);
    glClearColor (0.0f, 0.05f, 0.1f, 1.0f);
    glEnable(GL_COLOR_MATERIAL);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);


    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW: " << glewGetErrorString(err) << std::endl;
        exit(EXIT_FAILURE);
    }
    std::cout << "Using GLEW " << glewGetString(GLEW_VERSION) << std::endl;
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::cout << "Geometry Shaders supported: " << (GLEW_VERSION_3_2 ? "YES" : "NO") << std::endl; 

    // Créer l'amplificateur APRÈS l'initialisation de GLEW
    amplificator = new Amplification(planet);

    Palette::loadPalettes();
    planet.palette = Palette::getNextPallete();
    planet.generatePlates(nbPlates);
    planet.assignCrustParameters();

    mesh = planet;
    display_plates_mode = 1;
    display_normals = false;
    display_mesh = true;
    display_smooth_normals = true;
    displayMode = LIGHTED;
    display_directions = false;
    updateDisplayedColors();

    // Créer le shader d'atmosphère
    atmosphereShader = new ShaderProgram(
        "../shaders/atmosphere.vert",
        "../shaders/atmosphere.frag"
    );

    starsSkybox = new Skybox(
        "../shaders/stars.vert",
        "../shaders/stars.frag"
    );
    
    // Créer la sphère atmosphérique
    createAtmosphereSphere(1.1f, 64, atmosphereVAO, atmosphereVBO, atmosphereEBO, atmosphereIndexCount);
}


// ------------------------------------
// Rendering.
// ------------------------------------

void drawVector( Vec3 const & i_from, Vec3 const & i_to ) {

    glBegin(GL_LINES);
    glVertex3f( i_from[0] , i_from[1] , i_from[2] );
    glVertex3f( i_to[0] , i_to[1] , i_to[2] );
    glEnd();
}


void drawAtmosphere() {    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    
    atmosphereShader->use();
    
    // Matrices
    float projection[16], view[16], model[16];
    glGetFloatv(GL_PROJECTION_MATRIX, projection);
    glGetFloatv(GL_MODELVIEW_MATRIX, view);
    
    // Model = identité (planète centrée en 0)
    for (int i = 0; i < 16; i++) model[i] = (i % 5 == 0) ? 1.0f : 0.0f;
    
    atmosphereShader->setMat4("projection", projection);
    atmosphereShader->setMat4("view", view);
    atmosphereShader->setMat4("model", model);

    float camX, camY, camZ;
    camera.getPos(camX, camY, camZ);
    Vec3 camPos = Vec3(camX, camY, camZ);
    atmosphereShader->setVec3("cameraPosition", camPos[0], camPos[1], camPos[2]);

    // Paramètres de l'atmosphère - LÉGÈREMENT AJUSTÉS
    atmosphereShader->setVec3("planetCenter", 0.0f, 0.0f, 0.0f);
    atmosphereShader->setFloat("planetRadius", 1.0f); 
    atmosphereShader->setFloat("atmoRadius", planetRadiusMax * 1.12f); // AUGMENTÉ de 1.08 à 1.12
    
    // UTILISER la même direction du soleil que pour l'éclairage OpenGL
    Vec3 lightDir = sunDirection;
    lightDir.normalize();
    atmosphereShader->setVec3("lightDir", lightDir[0], lightDir[1], lightDir[2]);
    
    // Dessiner la sphère
    glBindVertexArray(atmosphereVAO);
    glDrawElements(GL_TRIANGLES, atmosphereIndexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    
    // Désactiver le shader
    glUseProgram(0);
    
    // Restaurer l'état OpenGL
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
}

void drawAxis( Vec3 const & i_origin, Vec3 const & i_direction ) {

    glLineWidth(4); // for example...
    drawVector(i_origin, i_origin + i_direction);
}

void drawReferenceFrame( Vec3 const & origin, Vec3 const & i, Vec3 const & j, Vec3 const & k ) {

    glDisable(GL_LIGHTING);
    glColor3f( 0.8, 0.2, 0.2 );
    drawAxis( origin, i );
    glColor3f( 0.2, 0.8, 0.2 );
    drawAxis( origin, j );
    glColor3f( 0.2, 0.2, 0.8 );
    drawAxis( origin, k );
    glEnable(GL_LIGHTING);

}






typedef struct {
    float r;       // ∈ [0, 1]
    float g;       // ∈ [0, 1]
    float b;       // ∈ [0, 1]
} RGB;



RGB scalarToRGB( float scalar_value ) //Scalar_value ∈ [0, 1]
{
    RGB rgb;
    float H = scalar_value*360., S = 1., V = 0.85,
            P, Q, T,
            fract;

    (H == 360.)?(H = 0.):(H /= 60.);
    fract = H - floor(H);

    P = V*(1. - S);
    Q = V*(1. - S*fract);
    T = V*(1. - S*(1. - fract));

    if      (0. <= H && H < 1.)
        rgb = (RGB){.r = V, .g = T, .b = P};
    else if (1. <= H && H < 2.)
        rgb = (RGB){.r = Q, .g = V, .b = P};
    else if (2. <= H && H < 3.)
        rgb = (RGB){.r = P, .g = V, .b = T};
    else if (3. <= H && H < 4.)
        rgb = (RGB){.r = P, .g = Q, .b = V};
    else if (4. <= H && H < 5.)
        rgb = (RGB){.r = T, .g = P, .b = V};
    else if (5. <= H && H < 6.)
        rgb = (RGB){.r = V, .g = P, .b = Q};
    else
        rgb = (RGB){.r = 0., .g = 0., .b = 0.};

    return rgb;
}

void drawSmoothTriangleMesh( Mesh const & i_mesh , bool draw_field = false ) {
    glBegin(GL_TRIANGLES);
    for(unsigned int tIt = 0 ; tIt < i_mesh.triangles.size(); ++tIt) {

        
        for(unsigned int i = 0 ; i < 3 ; i++) {
            const Vec3 & p = i_mesh.vertices[i_mesh.triangles[tIt][i]]; //Vertex position
            const Vec3 & n = i_mesh.normals[i_mesh.triangles[tIt][i]]; //Vertex normal
            const Vec3 & c = i_mesh.colors[i_mesh.triangles[tIt][i]];

            if( draw_field && current_field.size() > 0 ){
                RGB color = scalarToRGB( current_field[i_mesh.triangles[tIt][i]] );
                glColor3f( color.r, color.g, color.b );
            }
            glColor3f(c[0], c[1], c[2]);
            glNormal3f( n[0] , n[1] , n[2] );
            glVertex3f( p[0] , p[1] , p[2] );
        }
    }
    glEnd();

}

void drawTriangleMesh( Mesh const & i_mesh , bool draw_field = false  ) {
    glBegin(GL_TRIANGLES);
    for(unsigned int tIt = 0 ; tIt < i_mesh.triangles.size(); ++tIt) {
        const Vec3 & n = i_mesh.triangle_normals[ tIt ]; //Triangle normal
        for(unsigned int i = 0 ; i < 3 ; i++) {
            const Vec3 & p = i_mesh.vertices[i_mesh.triangles[tIt][i]]; //Vertex position

            if( draw_field ){
                RGB color = scalarToRGB( current_field[i_mesh.triangles[tIt][i]] );
                glColor3f( color.r, color.g, color.b );
            }
            Vec3 color = i_mesh.colors[i_mesh.triangles[tIt][i]];
            glColor3f(color[0], color[1], color[2]);
            glNormal3f( n[0] , n[1] , n[2] );
            glVertex3f( p[0] , p[1] , p[2] );
        }
    }
    glEnd();

}

void drawMesh( Mesh const & i_mesh , bool draw_field = false ){
    if(display_smooth_normals)
        drawSmoothTriangleMesh(i_mesh, draw_field) ; //Smooth display with vertices normals
    else
        drawTriangleMesh(i_mesh, draw_field) ; //Display with face normals
}




void drawVectorField( std::vector<Vec3> const & i_positions, std::vector<Vec3> const & i_directions ) {
    glLineWidth(1.);
    for(unsigned int pIt = 0 ; pIt < i_directions.size() ; ++pIt) {
        Vec3 to = i_positions[pIt] + 0.02*i_directions[pIt];
        drawVector(i_positions[pIt], to);
    }
}

void drawNormals(Mesh const& i_mesh){

    if(display_smooth_normals){
        drawVectorField( i_mesh.vertices, i_mesh.normals );
    } else {
        std::vector<Vec3> triangle_baricenters;
        for ( const Triangle& triangle : i_mesh.triangles ){
            Vec3 triangle_baricenter (0.,0.,0.);
            for( unsigned int i = 0 ; i < 3 ; i++ )
                triangle_baricenter += i_mesh.vertices[triangle[i]];
            triangle_baricenter /= 3.;
            triangle_baricenters.push_back(triangle_baricenter);
        }

        drawVectorField( triangle_baricenters, i_mesh.triangle_normals );
    }
}

//Draw fonction
void draw() {

    if (starsSkybox) {
        // Sauvegarder l'état actuel
        glPushAttrib(GL_ALL_ATTRIB_BITS);
        
        // Désactiver depth write et test temporairement
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_LIGHTING);
        glDisable(GL_CULL_FACE);
        
        // Récupérer les matrices
        float projection[16];
        float view[16];
        glGetFloatv(GL_PROJECTION_MATRIX, projection);
        glGetFloatv(GL_MODELVIEW_MATRIX, view);
        
        // Debug: afficher les matrices une fois
        static bool debugOnce = false;
        if (!debugOnce) {
            std::cout << "Projection[0]: " << projection[0] << std::endl;
            std::cout << "View[0]: " << view[0] << std::endl;
            debugOnce = true;
        }
        
        starsSkybox->draw(view, projection);
        
        // Restaurer l'état
        glPopAttrib();
        glEnable(GL_DEPTH_TEST);
    }

    // Le reste du code existant...
    if(displayMode == LIGHTED || displayMode == LIGHTED_WIRE){
        glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
        glEnable(GL_LIGHTING);
    } else if(displayMode == WIRE){
        glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
        glDisable (GL_LIGHTING);
    } else if(displayMode == SOLID ){
        glDisable (GL_LIGHTING);
        glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
    }

    glColor3f(0.8,1,0.8);
    drawMesh(mesh, true);

    if(displayMode == SOLID || displayMode == LIGHTED_WIRE){
        glEnable (GL_POLYGON_OFFSET_LINE);
        glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
        glLineWidth (1.0f);
        glPolygonOffset (-2.0, 1.0);

        glColor3f(0.,0.,0.);
        drawMesh(mesh, false);

        glDisable (GL_POLYGON_OFFSET_LINE);
        glEnable (GL_LIGHTING);
    }



    glDisable(GL_LIGHTING);
    if(display_normals){
        glColor3f(1.,0.,0.);
        drawNormals(mesh);
    }

    if(display_directions){
        drawPlateArrows(planet, 0.5f);
    }

    if (display_phenomena) {
        drawTectonicPhenomenaMarkers(planet, movement_controller.tectonicPhenomena, 0.02f);
    }

    if (display_atmosphere) {
        drawAtmosphere();
        glUseProgram(0);
    }

    


    glEnable(GL_LIGHTING);




}

void changeDisplayMode(){
    if(displayMode == LIGHTED)
        displayMode = LIGHTED_WIRE;
    else if(displayMode == LIGHTED_WIRE)
        displayMode = SOLID;
    else if(displayMode == SOLID)
        displayMode = WIRE;
    else{ 
        std::cout << "Switching to LIGHTED mode." << std::endl;
        displayMode = LIGHTED;
    
    }
}

void display () {
    glLoadIdentity ();
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    camera.apply ();
    draw ();
    glFlush ();
    glutSwapBuffers ();
}

void idle () {
    glutPostRedisplay ();
}

// ------------------------------------
// User inputs
// ------------------------------------
//Keyboard event
void key (unsigned char keyPressed, int x, int y) {
    switch (keyPressed) {
    case 'f':
        if (fullScreen == true) {
            glutReshapeWindow (SCREENWIDTH, SCREENHEIGHT);
            fullScreen = false;
        } else {
            glutFullScreen ();
            fullScreen = true;
        }
        break;


    case 'w':
        changeDisplayMode();
        break;

    case 'p': //Press p key to display plates
        display_plates_mode = (display_plates_mode + 1) % 4;
        updateDisplayedColors();
        break;

    case 'c': //Press c key to change colors
        planet.changePalette();
        updateDisplayedColors();
        break;

    case 'm': //Press m key to move the plates
        if (nbSteps < nbiter_resample) {
            movement_controller.movePlates(timeStep);
            erosion_controller.erosion();
            mesh = planet;
            updateDisplayedColors();
            elapsedSteps++;
            nbSteps++;
            printf("Moved plates: step %d\n", nbSteps);
        }else {
            nbSteps = 0;
            Planet newPlanet(1.0f,spherepoints);
            newPlanet.resample(planet);
            
            planet = std::move(newPlanet);
            
            
            movement_controller = Movement(planet);

            mesh = planet;
            updateDisplayedColors();

            printf("Resampled planet.\n");
        }

        /*for (int i = 0; i < planet.vertices.size(); i++) {
            float crust_elevation = planet.crust_data[i]->relief_elevation;
            float normalized_elevation = (crust_elevation - planet.min_elevation) / (planet.max_elevation - planet.min_elevation);
            mesh.vertices[i] = planet.vertices[i] * (1 + 0.2 * normalized_elevation);
        }*/

        break;

    case 'i': //trigger plat rifting
        {
            bool riftingOccurred = PlateRifting::triggerRifting(planet);
            if (riftingOccurred) {
                std::cout << "Rifting successful! Updating structures..." << std::endl;
                planet.findFrontierVertices();
                planet.fillClosestFrontierVertices();
                mesh = planet;
                updateDisplayedColors();
            } else {
                std::cout << "No rifting occurred." << std::endl;
            }
        }
        break;


    case 'k': // Rotate sun left
        {
            // Rotation autour de l'axe Y
            float angle = 0.1f;
            float newX = sunDirection[0] * cos(angle) - sunDirection[2] * sin(angle);
            float newZ = sunDirection[0] * sin(angle) + sunDirection[2] * cos(angle);
            sunDirection = Vec3(newX, sunDirection[1], newZ);
            initLight(); // Réappliquer l'éclairage
            //printf("Sun direction: (%.2f, %.2f, %.2f)\n", sunDirection[0], sunDirection[1], sunDirection[2]);
        }
        break;
        
    case 'l': // Rotate sun right
        {
            float angle = -0.1f;
            float newX = sunDirection[0] * cos(angle) - sunDirection[2] * sin(angle);
            float newZ = sunDirection[0] * sin(angle) + sunDirection[2] * cos(angle);
            sunDirection = Vec3(newX, sunDirection[1], newZ);
            initLight();
            //printf("Sun direction: (%.2f, %.2f, %.2f)\n", sunDirection[0], sunDirection[1], sunDirection[2]);
        }
        break;


    case 'o': // Rotate sun up
        {
            sunDirection[1] += 2.0f;
            initLight();
            //printf("Sun direction: (%.2f, %.2f, %.2f)\n", sunDirection[0], sunDirection[1], sunDirection[2]);
        }
        break;
        
    case 'u': // Rotate sun down
        {
            sunDirection[1] -= 2.0f;
            initLight();
            //printf("Sun direction: (%.2f, %.2f, %.2f)\n", sunDirection[0], sunDirection[1], sunDirection[2]);
        }
        break;

    case 'b': // toggle subduction markers and compute once
        display_phenomena = !display_phenomena;
        if (display_phenomena) {
            printf("Detected %zu plate interaction points\n", movement_controller.tectonicPhenomena.size());
            printf("Subduction markers ON (%zu candidates)\n", movement_controller.tectonicPhenomena.size());
        } else {
            printf("Subduction markers OFF\n");
        }
        break;

    case 'h': // toggle atmosphere display
        display_atmosphere = !display_atmosphere;
        if (display_atmosphere) {
            printf("Atmosphere display ON\n");
        } else {
            printf("Atmosphere display OFF\n");
        }
        break;

    case 'n': //Press n key to display normals
        display_normals = !display_normals;
        break;

    case 's': //Press s key to smooth
        planet.smoothColors();
        mesh = planet;
        break;

    case 'r'://resample
        {
            Planet newPlanet(1.0f,spherepoints);
            newPlanet.resample(planet);

            planet = std::move(newPlanet);

            movement_controller.planet = &planet;
            planet.detectVerticesNeighbors();
            
            mesh = planet;
            updateDisplayedColors();
            nbSteps = 0;
            printf("Resampled planet.\n");
        }
        break;

    case 'a': // Amplify
        amplificator->amplifyTerrain(planet);
        display_plates_mode = 3;
        
        amplifiedPlanetRadius = planet.computeAverageDistanceFromOrigin();
        planetRadiusMin = planet.computeMinDistanceFromOrigin();
        planetRadiusMax = planet.computeMaxDistanceFromOrigin();
        
        std::cout << "Planet radius - Min: " << planetRadiusMin 
                << " Avg: " << amplifiedPlanetRadius 
                << " Max: " << planetRadiusMax << std::endl;
        

        if (atmosphereVAO != 0) {
            glDeleteVertexArrays(1, &atmosphereVAO);
            glDeleteBuffers(1, &atmosphereVBO);
            glDeleteBuffers(1, &atmosphereEBO);
        }
        createAtmosphereSphere(planetRadiusMax * 1.12f, 64, atmosphereVAO, atmosphereVBO, atmosphereEBO, atmosphereIndexCount);
        
        mesh = planet;
        displayMode = LIGHTED;
        display_atmosphere = true;
        
        updateDisplayedColors();
        break;

    case 'j': //relancer
    {

        if (amplificator) {
            delete amplificator;
            amplificator = nullptr;
        }

        Planet newPlanet(1.0f, spherepoints);
    
        newPlanet.generatePlates(nbPlates);
        newPlanet.assignCrustParameters();  

        planet = std::move(newPlanet);  

        amplificator = new Amplification(planet);  

        movement_controller = Movement(planet); 

        display_plates_mode = 1;
        displayMode = LIGHTED;
        display_atmosphere = true;

        amplifiedPlanetRadius = 1.0f;
        planetRadiusMin = 1.0f;
        planetRadiusMax = 1.0f; 

        if (atmosphereVAO != 0) {
            glDeleteVertexArrays(1, &atmosphereVAO);
            glDeleteBuffers(1, &atmosphereVBO);
            glDeleteBuffers(1, &atmosphereEBO);
        }
        createAtmosphereSphere(1.1f, 64, atmosphereVAO, atmosphereVBO, atmosphereEBO, atmosphereIndexCount);

        mesh = planet;
        updateDisplayedColors();
        
        nbSteps = 0;
        elapsedSteps = 1.0f;
        
        printf("Restarted simulation completely.\n");
    }
    break;

    case '1': //Toggle loaded mesh display
        display_mesh = !display_mesh;
        break;

    case 'd':
        display_directions = !display_directions;
        break;


    default:
        break;
    }
    idle ();
}

//Mouse events
void mouse (int button, int state, int x, int y) {
    if (state == GLUT_UP) {
        mouseMovePressed = false;
        mouseRotatePressed = false;
        mouseZoomPressed = false;
    } else {
        if (button == GLUT_LEFT_BUTTON) {
            camera.beginRotate (x, y);
            mouseMovePressed = false;
            mouseRotatePressed = true;
            mouseZoomPressed = false;
        } else if (button == GLUT_RIGHT_BUTTON) {
            lastX = x;
            lastY = y;
            mouseMovePressed = true;
            mouseRotatePressed = false;
            mouseZoomPressed = false;
        } else if (button == GLUT_MIDDLE_BUTTON) {
            if (mouseZoomPressed == false) {
                lastZoom = y;
                mouseMovePressed = false;
                mouseRotatePressed = false;
                mouseZoomPressed = true;
            }
        }
    }

    idle ();
}

//Mouse motion, update camera
void motion (int x, int y) {
    if (mouseRotatePressed == true) {
        camera.rotate (x, y);
    }
    else if (mouseMovePressed == true) {
        camera.move ((x-lastX)/static_cast<float>(SCREENWIDTH), (lastY-y)/static_cast<float>(SCREENHEIGHT), 0.0);
        lastX = x;
        lastY = y;
    }
    else if (mouseZoomPressed == true) {
        camera.zoom (float (y-lastZoom)/SCREENHEIGHT);
        lastZoom = y;
    }
}


void reshape(int w, int h) {
    camera.resize (w, h);
}

// ------------------------------------
// Start of graphical application
// ------------------------------------
int main (int argc, char ** argv) {
    if (argc > 2) {
        exit (EXIT_FAILURE);
    }

    std::cout << "----------------------------------------" << std::endl;
    std::cout << "Procedural Planet with Tectonic Plates Simulation" << std::endl;
    glutInit (&argc, argv);
    glutInitDisplayMode (GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
    glutInitWindowSize (SCREENWIDTH, SCREENHEIGHT);
    window = glutCreateWindow ("Procedural Planet with Tectonic Plates Simulation");

    init ();
    glutIdleFunc (idle);
    glutDisplayFunc (display);
    glutKeyboardFunc (key);
    glutReshapeFunc (reshape);
    glutMotionFunc (motion);
    glutMouseFunc (mouse);
    key ('?', 0, 0);

    //Mesh loaded with precomputed normals




    // A faire : normaliser les champs pour avoir une valeur flotante entre 0. et 1. dans current_field
    //***********************************************//

    current_field.clear();

    glutMainLoop ();

    delete amplificator;
    delete atmosphereShader;
    delete starsSkybox;
    
    return EXIT_SUCCESS;
}

