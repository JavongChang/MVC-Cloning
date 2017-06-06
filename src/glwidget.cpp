#include "glwidget.h"
#include "mvcshaders.h"
#include "utils.h"
#include <QtGui>
#include <QtOpenGL>

#include <math.h>
#include "utils.h"

#include <QCoreApplication>
#include <GL/glut.h>

#include <QtCore>
//#include "glext.h"

GLWidget::GLWidget(QWidget *parent)
: QGLWidget(parent){
    
    target_w = 400;
    target_h = 400;
    
    init = false;
    mode = MVC;
    antsOn = true;
    antColor = 1.0;			
    
    selection.vertices = 0;
    
    selection.tx = 0;
    selection.ty = 0;
    selection.dx = 0;
    selection.dy = 0;
	
	selection.rangle = 0.0;
	selection.flip = 0.0;
	selection.scale = 1.0;	
	
    method = HIER1;
	
	sourceImage = NULL;
	alphaImage = NULL;
	targetImage = NULL;

	//countnum = 0;

	//pLabel = new QLabel(this);
	//pLabel->setStyleSheet("color: white");
	//pLabel->setGeometry(QRect(20, 20, 150, 30));
	//timer = new QTimer(this);
	//timer->setInterval(10); //1000ms == 1s
	//connect(timer, SIGNAL(timeout()), this, SLOT(count()));
	//timer->start();
}

GLWidget::~GLWidget(){
    
	// Instantiated only once. Let OS do the cleaning
	delete pLabel;
	delete timer;
    makeCurrent();
}

void GLWidget::initializeGL(){
    
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glDisable(GL_DEPTH_TEST);
    
    target = GL_TEXTURE_RECTANGLE_ARB;
	//target = GL_TEXTURE_2D;
    glEnable(target);
    
    loadShaders();
    showMVC();
}

void GLWidget::loadShaders(){
	
    if (method==HIER1){
		compileAttachLinkShaderFromSource(getSamplingVS(), getSamplingFS());
		//compileAttachLinkShaderFromSource(":/src/SamplingVS.glsl", ":/src/getSamplingFS.glsl");
		return;
    }
    if (method==HIER2){	
		compileAttachLinkShaderFromSource(getSampling2DFilterVS(), getSamplingFS());
		return;
    }	
    if (method==ADAP1 ||method==ADAP2){
		compileAttachLinkShaderFromSource(getAlphaVS(), getAlphaFS());
		return;
    }		
}

void GLWidget::compileAttachLinkShaderFromSource(const QString& vs, const QString& fs) {
    m_program = new QOpenGLShaderProgram(this);

    if (!m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vs)) {
        qWarning("Vertex shader compilation failed");
		//QMessageBox::warning(this,"x",m_program->log());
    }

    if (!m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fs)) {
        qWarning("Fragment shader compilation failed");
    }

    if (!m_program->link()) {
		qWarning("Failed to compile and link shader program");
		qWarning("Shader program log:");
		qWarning() << m_program->log();
		delete m_program;
	}
}

void GLWidget::count()
{
	countnum++;
}

void GLWidget::paintGL(){

	//timer = new QTimer(this);
	//timer->setInterval(1); //1000ms == 1s
	//connect(timer, SIGNAL(timeout()), this, SLOT(count()));
	//timer->start(10);
    glClear(GL_COLOR_BUFFER_BIT);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    

    //glEnable(GL_TEXTURE0_ARB);
    // Blit the target image
    /**********************************/
    PFNGLMULTITEXCOORD2FARBPROC glMultiTexCoord2fARB = NULL;
    glMultiTexCoord2fARB     = (PFNGLMULTITEXCOORD2FARBPROC)  wglGetProcAddress("glMultiTexCoord2fARB");
    /***********************************/

    glBegin(GL_QUADS);
    glMultiTexCoord2fARB(GL_TEXTURE0_ARB,0,0);
    glVertex2f(0.0, 0.0);
    glMultiTexCoord2fARB(GL_TEXTURE0_ARB,target_w,0);
    glVertex2f(target_w, 0);
    glMultiTexCoord2fARB(GL_TEXTURE0_ARB,target_w,target_h);
    glVertex2f(target_w, target_h);
    glMultiTexCoord2fARB(GL_TEXTURE0_ARB,0,target_h);
    glVertex2f(0, target_h);
    glEnd();

/*
    glBegin(GL_QUADS);
      glMultiTexCoord2fARB(GL_TEXTURE2_ARB,0.0, 0.0);
      glVertex2f(100.0, 100.0);
      glMultiTexCoord2fARB(GL_TEXTURE2_ARB,0.0, 100);
      glVertex2f(100.0, 200.0);
      glMultiTexCoord2fARB(GL_TEXTURE2_ARB,100, 100);
      glVertex2f(200.0, 200.0);
      glMultiTexCoord2fARB(GL_TEXTURE2_ARB,100, 0.0);
      glVertex2f(200.0, 100.0);
    glEnd();
*/

    // Transform the selection patch
	glTranslatef(selection.tx+selection.dx, selection.ty+selection.dy, 0);
	glTranslatef(-selection.toOrigin.x, -selection.toOrigin.y, 0);
	glRotatef(selection.rangle,0,0,1);
	glTranslatef(selection.toOrigin.x, selection.toOrigin.y, 0);
	
	glTranslatef(-selection.toOrigin.x, -selection.toOrigin.y, 0);
	if (selection.flip)
		glScalef(-1*selection.scale,1*selection.scale,1);
	else
		glScalef(selection.scale,1*selection.scale,1);
	
	glTranslatef(selection.toOrigin.x, selection.toOrigin.y, 0);
    
    // Show the selection (according to the viewing mode)
    if (mode==TRI)
        paintMesh();
	else
		paintSelection();
	

	
	//pLabel->clear();
	//pLabel->setText(QString::number(countnum));
	//pLabel->setNum(countnum);
	
	
	/*pLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	pLabel->setAlignment(Qt::AlignBottom | Qt::AlignRight);*/
	
	//timer->stop();
}

void GLWidget::paintSelection(){
    
    if (!init)
        return;

    // Enable the GLSL program
    m_program->bind();


    //GLint texLoc;
    //texLoc = m_program->glGetUniformLocation(m_program->shaders(), "tex0_id");
    //glUniform1i(texLoc, 0); //GL_TEXTURE0,


    // and pass data inside
 /*   m_program->setAttributeValue("tex0", 0);
    m_program->setAttributeValue("tex1", 1);
    m_program->setAttributeValue("tex2", 2);
    m_program->setAttributeValue("tex3", 3);
	m_program->setAttributeValue("tex4", 4);
    m_program->setAttributeValue("mode", mode);*/

	m_program->setUniformValue("tex0", 0);
	m_program->setUniformValue("tex1", 1);
	m_program->setUniformValue("tex2", 2);
	m_program->setUniformValue("tex3", 3);
	m_program->setUniformValue("tex4", 4);
	m_program->setUniformValue("mode", mode);
	//m_program->setUniformValue(m_program->uniformLocation("mode"), mode);

    /*if(method==HIER1 || method==HIER2){
        m_program->setAttributeValue("target_h", target_h);
        m_program->setAttributeValue("source_h", selection_h);
    }else if(method==ADAP1){
        m_program->setAttributeValue("blend", 0);
        m_program->setAttributeValue("boundarySize", selection.boundarySize);
	}else if(method==ADAP2){
        m_program->setAttributeValue("blend", 1);
        m_program->setAttributeValue("boundarySize", selection.boundarySize);
    }*/

    float fx=selection.tx+selection.dx;
    float fy=selection.ty+selection.dy;

    glNormal3f(fx, fy, 0);
	
    Vertex2D v1, v2, v3;
    int ind1, ind2, ind3;


    /*******************************/
    PFNGLACTIVETEXTUREARBPROC glActiveTextureARB = NULL;
    PFNGLMULTITEXCOORD2FARBPROC glMultiTexCoord2fARB = NULL;
    glMultiTexCoord2fARB     = (PFNGLMULTITEXCOORD2FARBPROC)  wglGetProcAddress("glMultiTexCoord2fARB");
    glActiveTextureARB  = (PFNGLACTIVETEXTUREARBPROC)  wglGetProcAddress("glActiveTextureARB");
    /*******************************/

    //??????????????????          test           ??????????????????????

/*
   glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tex1_id);//绑定纹理目标
        glBegin(GL_QUADS);
            glTexCoord2f( 100, 100 ); glVertex2f( 100, 100);
            glTexCoord2f( 100, 200 ); glVertex2f( 100, 200);
            glTexCoord2f( 200, 200 ); glVertex2f( 200, 200);
            glTexCoord2f( 200, 100 ); glVertex2f( 200, 100);
        glEnd();
*/


   /*
    glBegin(GL_QUADS);
      glMultiTexCoord2fARB(GL_TEXTURE0_ARB,0.0, 0.0);
      glVertex2f(100.0, 100.0);
      glMultiTexCoord2fARB(GL_TEXTURE0_ARB,0.0, 100);
      glVertex2f(100.0, 200.0);
      glMultiTexCoord2fARB(GL_TEXTURE0_ARB,100, 100);
      glVertex2f(200.0, 200.0);
      glMultiTexCoord2fARB(GL_TEXTURE0_ARB,100, 0.0);
      glVertex2f(200.0, 100.0);
    glEnd();
    */

    for (int i=0; i<selection.numTriangles; i++){
        
        ind1 = selection.triangles[i].p1;
        ind2 = selection.triangles[i].p2;
        ind3 = selection.triangles[i].p3;
        
        v1 = selection.vertices[ind1];
        v2 = selection.vertices[ind2];
        v3 = selection.vertices[ind3];



        /*
        target = GL_TEXTURE_RECTANGLE_ARB;
        glActiveTextureARB (GL_TEXTURE0_ARB);
        glEnable(target);
        glBindTexture(target, tex0_id);
        //glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);//替换
        target = GL_TEXTURE_RECTANGLE_ARB;
        glActiveTextureARB (GL_TEXTURE1_ARB);
        glEnable(target);
        glBindTexture(target, tex1_id);
        //glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);//替换
*/

       // glEnable(GL_TEXTURE_RECTANGLE_ARB);
        glEnable(GL_BLEND);

        glBegin(GL_TRIANGLES);
        

        glMultiTexCoord2fARB(GL_TEXTURE0_ARB, v1.x+fx, v1.y+fy);
        glMultiTexCoord2fARB(GL_TEXTURE1_ARB, v1.x, v1.y);
        glMultiTexCoord2fARB(GL_TEXTURE2_ARB, 0, ind1);
        glMultiTexCoord2fARB(GL_TEXTURE3_ARB, 0, 0);
		glMultiTexCoord2fARB(GL_TEXTURE4_ARB, v1.x, v1.y);
        glVertex2f(v1.x, v1.y);
        

        glMultiTexCoord2fARB(GL_TEXTURE0_ARB, v2.x+fx, v2.y+fy);
        glMultiTexCoord2fARB(GL_TEXTURE1_ARB, v2.x, v2.y);
        glMultiTexCoord2fARB(GL_TEXTURE2_ARB, 0, ind2);
        glMultiTexCoord2fARB(GL_TEXTURE3_ARB, 0, 0);
		glMultiTexCoord2fARB(GL_TEXTURE4_ARB, v2.x, v2.y);
        glVertex2f(v2.x, v2.y);
        

        glMultiTexCoord2fARB(GL_TEXTURE0_ARB, v3.x+fx, v3.y+fy);
        glMultiTexCoord2fARB(GL_TEXTURE1_ARB, v3.x, v3.y);
        glMultiTexCoord2fARB(GL_TEXTURE2_ARB, 0, ind3);
        glMultiTexCoord2fARB(GL_TEXTURE3_ARB, 0, 0);
		glMultiTexCoord2fARB(GL_TEXTURE4_ARB, v3.x, v3.y);
        glVertex2f(v3.x, v3.y);
        
        glEnd();

    }


    //m_program->release();
}

void GLWidget::paintMesh(){
    
    if (!init)
        return;
    
    int ind1, ind2, ind3;
    Vertex2D v1, v2, v3;
    
    for (int i=0; i<selection.numTriangles; i++){
        
        glBegin(GL_LINES);
        ind1 = selection.triangles[i].p1;
        ind2 = selection.triangles[i].p2;
        ind3 = selection.triangles[i].p3;
        
        v1 = selection.vertices[ind1];
        v2 = selection.vertices[ind2];
        v3 = selection.vertices[ind3];
        
        glVertex2f(v1.x, v1.y);
        glVertex2f(v2.x, v2.y);
        glVertex2f(v2.x, v2.y);
        glVertex2f(v3.x, v3.y);
        glVertex2f(v3.x, v3.y);
        glVertex2f(v1.x, v1.y);
        
        glEnd();
    }
}

void GLWidget::resizeGL(int width, int height){
    
    /* Identity mapping between image and GL coordinates */
    glViewport(0, 0, (GLsizei)width, (GLsizei)height) ;
    glMatrixMode(GL_PROJECTION) ;
    glLoadIdentity() ;
    gluOrtho2D(0, (GLdouble)width, 0, (GLdouble)height) ;
    
}

void GLWidget::mousePressEvent(QMouseEvent *event){
    
    if (event->buttons() & Qt::LeftButton) {
        click_x = event->pos().x();
        click_y = event->pos().y();
    }
    update();
}

void GLWidget::mouseReleaseEvent(QMouseEvent *event){
    
    if (event->button() == Qt::LeftButton) {
        
        click_x = 0;
        click_y = 0;
        
        selection.tx += selection.dx;
        selection.ty += selection.dy;
        
        selection.dx = 0;
        selection.dy = 0;
    }
    update();
}

void GLWidget::mouseMoveEvent(QMouseEvent *event){
    
    if (event->buttons() & Qt::LeftButton) {
        selection.dx = event->pos().x()-click_x;
        selection.dy = click_y-event->pos().y();
    }
    update();
}

void GLWidget::bindTarget(){
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &tex0_id);
    
    /*******************************/
    PFNGLACTIVETEXTUREARBPROC glActiveTextureARB = NULL;
    PFNGLMULTITEXCOORD2FARBPROC glMultiTexCoord2fARB = NULL;
    glMultiTexCoord2fARB     = (PFNGLMULTITEXCOORD2FARBPROC)  wglGetProcAddress("glMultiTexCoord2fARB");
    glActiveTextureARB  = (PFNGLACTIVETEXTUREARBPROC)  wglGetProcAddress("glActiveTextureARB");
    /*******************************/


    //target = GL_TEXTURE_RECTANGLE_ARB;
    glActiveTextureARB(GL_TEXTURE0_ARB);
    //glEnable(GL_TEXTURE_RECTANGLE_ARB);
    glBindTexture(target, tex0_id);
    glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP);
    // Don't filter
    glTexParameteri(target,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(target,GL_TEXTURE_MIN_FILTER,GL_NEAREST);

    
	if (method==HIER2) {
        // Create a mipmap - used in HIER2 method
		QImage * res = createMipMap(*targetImage);
		glTexImage2D(target, 0, GL_RGBA, target_w, res->height(), 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, res->bits());
		delete res;
	}
	else
		glTexImage2D(target, 0, GL_RGBA, target_w, target_h, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, targetImage->mirrored().bits());	

}

void GLWidget::bindSource(){
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &tex1_id);
    
    /*******************************/
    PFNGLACTIVETEXTUREARBPROC glActiveTextureARB = NULL;
    PFNGLMULTITEXCOORD2FARBPROC glMultiTexCoord2fARB = NULL;
    glMultiTexCoord2fARB     = (PFNGLMULTITEXCOORD2FARBPROC)  wglGetProcAddress("glMultiTexCoord2fARB");
    glActiveTextureARB  = (PFNGLACTIVETEXTUREARBPROC)  wglGetProcAddress("glActiveTextureARB");
    /*******************************/


    //target = GL_TEXTURE_RECTANGLE_ARB;
    glActiveTextureARB(GL_TEXTURE1_ARB);
    //glEnable(GL_TEXTURE_RECTANGLE_NV);
    //glEnable(GL_TEXTURE_RECTANGLE_ARB);
    glBindTexture(target, tex1_id);
    glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP);
    // Don't filter
    glTexParameteri(target,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(target,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    
	if (method==HIER2) {
		// Create a mipmap - used in HIER2 method
		QImage * res = createMipMap(*sourceImage);
        glTexImage2D(target, 0, GL_RGBA, sourceImage->width(), res->height(), 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, res->bits());
		delete res;
	}
	else
        glTexImage2D(target, 0, GL_RGBA, sourceImage->width(), sourceImage->height(), 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, sourceImage->mirrored().bits());



}

void GLWidget::bindAlpha(){
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &tex4_id);
    
    /*******************************/
    PFNGLACTIVETEXTUREARBPROC glActiveTextureARB = NULL;
    PFNGLMULTITEXCOORD2FARBPROC glMultiTexCoord2fARB = NULL;
    glMultiTexCoord2fARB     = (PFNGLMULTITEXCOORD2FARBPROC)  wglGetProcAddress("glMultiTexCoord2fARB");
    glActiveTextureARB  = (PFNGLACTIVETEXTUREARBPROC)  wglGetProcAddress("glActiveTextureARB");
    /*******************************/


    //target = GL_TEXTURE_RECTANGLE_ARB;
    glActiveTextureARB(GL_TEXTURE4_ARB);
    //glEnable(GL_TEXTURE_RECTANGLE_NV);
    //glEnable(GL_TEXTURE_RECTANGLE_ARB);
    glBindTexture(target, tex4_id);
    glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP);
    // Don't filter
    glTexParameteri(target,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(target,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    
	if (method==HIER2) {
		// Create a mipmap - used in HIER2 method
		QImage * res = createMipMap(*alphaImage);
        glTexImage2D(target, 0, GL_RGBA, alphaImage->width(), res->height(), 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, res->bits());
		delete res;
	}
	else
        glTexImage2D(target, 0, GL_RGBA, alphaImage->width(), alphaImage->height(), 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, alphaImage->mirrored().bits());



}

void GLWidget::setTargetImage(const QImage & image){ 
	
	if (targetImage)
		delete targetImage;
	
	targetImage = new QImage(image.copy());
    target_w = image.width();
    target_h = image.height();
	
	bindTarget();
}

void GLWidget::setSourcePatch(const QImage & image)
{
	if (sourceImage)
		delete sourceImage;	
    
	sourceImage = new QImage(image.copy());
    selection_w = image.width();
    selection_h = image.height();
    
	bindSource();
}


void GLWidget::setAlphaPatch(const QImage & image)
{
	if (alphaImage)
		delete alphaImage;	
    
	alphaImage = new QImage(image.copy());
    alpha_w = image.width();
    alpha_h = image.height();
    
	bindAlpha();
}
/* Gets boundary and the selection polygon(needed for triangle inside/outside test).
 Converts CGAL data-structures to data usable for shaders.
 
 TODO: very inefficient at the moment.
 Due to the fact that we proably miss some bits form the CGAL documentation
 the methods does a lot of probably unnessecary work and it's a major bottleneck
 right now.
 
 */
void GLWidget::updateSelection(std::vector<Point> & boundaryVector, QPolygonF & selectionPoly, bool reset)
{
    
	// Copy the data (we needed it in order to change the settings on the fly)
	this->selectionPoly = selectionPoly;
	this->boundaryVector = boundaryVector;
	
	//qDebug() << boundaryVector.size() << " " << selectionPoly.size();
	
	if (reset)
		resetSelection();
	
    MVCCloner cloner;
    CloningParameters params;	
    
    if (method==HIER1 || method==HIER2)
        params.setHierarchic(); 	
    
    // TODO: Ask Gil how about freeing the memory of those guys
    // TODO: CGAL still throws something this thing doesnt catch
    try {		
        cmesh = cloner.preprocess(boundaryVector,params);
        adaptiveMesh = cmesh->mesh;
		cloner.tagFaces(adaptiveMesh);
    }
    catch (...)
    {
        qDebug() << "CGAL is not happy with the selection";
    }
    
    // Clean/allocate selection
    if (selection.vertices){ //sufficient indication for the rest
        delete[] selection.vertices;
        delete[] selection.triangles;
        delete[] selection.boundaryCoordsTex;
        delete[] selection.weightsTex;
    }
    selection.boundarySize = boundaryVector.size();
    selection.numPoints = adaptiveMesh->number_of_vertices();
    selection.numTriangles = adaptiveMesh->number_of_faces();
    
    selection.vertices = new Vertex2D[selection.numPoints];        
    selection.boundaryCoordsTex = new float[selection.boundarySize * 2];
    selection.weightsTex = new float[selection.boundarySize * selection.numPoints];
    
    // Associate a number with each vertex
    std::map<Point, int> mapping;
    std::list<Point> orderedPoints;
    int index = 0;
    for (FiniteFacesIterator iter = adaptiveMesh->finite_faces_begin() ; iter != adaptiveMesh->finite_faces_end() ; ++iter) {
        Triangle triangle = adaptiveMesh->triangle(iter);
        Point v1 = triangle.vertex(0);
        if (mapping.find(v1) == mapping.end()) {
            mapping[v1] = index++;
            orderedPoints.push_back(v1);
        }
        Point v2 = triangle.vertex(1);
        if (mapping.find(v2) == mapping.end()) {
            mapping[v2] = index++;
            orderedPoints.push_back(v2);
        }
        Point v3 = triangle.vertex(2);
        
        if (mapping.find(v3) == mapping.end()) {
            mapping[v3] = index++;
            orderedPoints.push_back(v3);
        }
    }
    
    // Verices
    int i = 0;
    for (std::list<Point>::const_iterator iter = orderedPoints.begin(); iter != orderedPoints.end() ; iter++)
    {
        selection.vertices[i].x = iter->x() ;
        selection.vertices[i].y = iter->y() ;
        i++;
    }
	
	
    // Triangles
	// Find inside triangles
	bool * trianglesToSkip = new bool[selection.numTriangles];
	selection.numTriangles = 0;
    i = 0;
	
	QRectF bb = selectionPoly.boundingRect();
	QPolygonF rect(bb);
	    
    for (FiniteFacesIterator iter = adaptiveMesh->finite_faces_begin() ; iter != adaptiveMesh->finite_faces_end() ; ++iter)
    {	
        if ((*iter).is_in_domain()) {
			selection.numTriangles++;
            trianglesToSkip[i] = false;
		}
        else
            trianglesToSkip[i] = true;
        
        i++;
    }
	selection.triangles = new Triangles[selection.numTriangles];
	qDebug() << selection.numTriangles << " inside the selection.";
    i = 0; int j = 0;
	// Add triangles

    for (FiniteFacesIterator iter = adaptiveMesh->finite_faces_begin() ; iter != adaptiveMesh->finite_faces_end() ; ++iter)
    {
        const Triangle &triangle = adaptiveMesh->triangle(iter);
        Point v1 = triangle.vertex(0);
        int index1 = mapping[v1];
        Point v2 = triangle.vertex(1);
        int index2 = mapping[v2];
        Point v3 = triangle.vertex(2);
        int index3 = mapping[v3];
        
		if (!trianglesToSkip[i]) {
			selection.triangles[j].p1 = index1;
			selection.triangles[j].p2 = index2;
			selection.triangles[j].p3 = index3;
			j++;
		}
		i++;
    }	
    delete [] trianglesToSkip;
	
    // Boundary
	float meanX = 0;
	float meanY = 0;	
    i = 0;
    for(std::vector<Point>::const_iterator it = boundaryVector.begin(); it != boundaryVector.end(); ++it){
        selection.boundaryCoordsTex[i] = (*it).x();
        selection.boundaryCoordsTex[selection.boundarySize + i] = (*it).y();
        i++;
		
		meanX += (*it).x();
		meanY += (*it).y();	
    }
	
	selection.toOrigin.x = -meanX/selection.boundarySize;
	selection.toOrigin.y = -meanY/selection.boundarySize;
	
    /*******************************/
    PFNGLACTIVETEXTUREARBPROC glActiveTextureARB = NULL;
    PFNGLMULTITEXCOORD2FARBPROC glMultiTexCoord2fARB = NULL;
    glMultiTexCoord2fARB     = (PFNGLMULTITEXCOORD2FARBPROC)  wglGetProcAddress("glMultiTexCoord2fARB");
    glActiveTextureARB  = (PFNGLACTIVETEXTUREARBPROC)  wglGetProcAddress("glActiveTextureARB");
    /*******************************/

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    //target = GL_TEXTURE_RECTANGLE_ARB;
    glGenTextures(1, &tex3_id);
    glActiveTextureARB(GL_TEXTURE3_ARB);
    //glEnable(GL_TEXTURE_RECTANGLE_ARB);
    glBindTexture(target, tex3_id);

	// This causes Error 1280
//    glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_REPEAT);
//    glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // Don't filter
    glTexParameteri(target,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(target,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexImage2D(target, 0, GL_LUMINANCE32F_ARB, selection.boundarySize, 2, 0, GL_LUMINANCE, GL_FLOAT, selection.boundaryCoordsTex);
    // errCheck("tex3_id");
    check_gl_error();

    // Weights
    if (method==HIER1 || method==HIER2)	{
		int maxRelevantWeights = cloner.maxRelevantWeights;
		float * WID  = new float[maxRelevantWeights * selection.numPoints * 3];	
		
		//i=0;
		for (std::map<VertexHandle,HierarchicCoordinateVector*>::const_iterator iter = cmesh->beginVertexToHierarchicCoordinates() ;
			 iter != cmesh->endVertexToHierarchicCoordinates() ; ++iter) {
			Point v = iter->first->point();
			i = mapping[v];
			
			HierarchicCoordinateVector* hcv = iter->second;
			
			int numberOfCoords = hcv->getSize();
			//qDebug() << numberOfCoords << "  " <<  maxRelevantWeights;
			assert(numberOfCoords<=maxRelevantWeights);
			double* coords = hcv->getCoords();
			int* indices = hcv->getIndices();
			int* depths = hcv->getDepths();
			
			WID[(i*maxRelevantWeights)*3] = numberOfCoords;
			WID[(i*maxRelevantWeights)*3+1] = numberOfCoords;
			WID[(i*maxRelevantWeights)*3+2] = numberOfCoords;
			
			for (int j=0; j<numberOfCoords; j++){
				//qDebug() << indices[j];
				assert(indices[j]<selection.boundarySize);
				assert(coords[j]<=1.0);
				WID[(i*maxRelevantWeights + j+1)*3] = coords[j];
				if (coords[j]==1.0)
					qDebug() << coords[j];
				WID[(i*maxRelevantWeights + j+1)*3+1] = indices[j];
				WID[(i*maxRelevantWeights + j+1)*3+2] = depths[j];
			}
		}	

		
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        // Upload to GPU
        //target = GL_TEXTURE_RECTANGLE_ARB;
        glGenTextures(1, &tex2_id);
        glActiveTextureARB(GL_TEXTURE2_ARB);
        //glEnable(GL_TEXTURE_RECTANGLE_ARB);
        glBindTexture(target, tex2_id);
        //
		glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP);
		// Don't filter
		glTexParameteri(target,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
		glTexParameteri(target,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
		glTexImage2D(target, 0, GL_RGB32F_ARB, maxRelevantWeights, selection.numPoints, 0, GL_RGB, GL_FLOAT, WID) ;
        // errCheck(QString("Line 579"));
        check_gl_error();
        delete[] WID;
    }
    else {
        
        
		for (std::map<VertexHandle,double*>::const_iterator iter = cmesh->beginVertexToCoordinates() ; iter != cmesh->endVertexToCoordinates() ; ++iter) {
			// Index correspond to the position in the texture
			Point v = (*iter).first->point();
			int index = mapping[v];
			for (int j=0; j<selection.boundarySize; j++){
				selection.weightsTex[index*selection.boundarySize + j] = (*iter).second[j];
			}
		}
		

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        // Upload to GPU

        //target = GL_TEXTURE_RECTANGLE_ARB;
        glGenTextures(1, &tex2_id);
        glActiveTextureARB(GL_TEXTURE2_ARB);
        //glEnable(GL_TEXTURE_RECTANGLE_NV);
        //glEnable(GL_TEXTURE_RECTANGLE_ARB);
        glBindTexture(target, tex2_id);

        glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP);
        // Don't filter
        glTexParameteri(target,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        glTexParameteri(target,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        glTexImage2D(target, 0, GL_LUMINANCE32F_ARB, selection.boundarySize, selection.numPoints, 0, GL_LUMINANCE, GL_FLOAT, selection.weightsTex) ;
        //glTexImage2D(target, 0, GL_LUMINANCE32F_ARB, selection.boundarySize, 2, 0, GL_LUMINANCE, GL_FLOAT, selection.boundaryCoordsTex);

		errCheck();
	}
	
    init = true;
    //paintGL();
	QString message = tr("Drag the selection around the target image. For more help, see: about MVCloner");
	((QMainWindow *)this->parentWidget())->statusBar()->showMessage(message);
}

void GLWidget::errCheck(QString mymsg){
    GLenum e;
    if ((e=glGetError()) != GL_NO_ERROR)
        qDebug()  << mymsg << " Error " << e;
} 


void GLWidget::setMethod(int newMethod) {
	
	switch(newMethod)
    {
    case 0:
		if (method==HIER1)
			return;
		else
			method=HIER1;
        break;
			
	case 1:
		if (method==HIER2)
			return;
		else {
			method=HIER2;
			// Create mipmaps
			bindSource();
			bindTarget();
		}
		break;
			
	case 2:
		if (method==ADAP1)
			return;
		else
			method=ADAP1;	
		break;
			
	case 3:
		if (method==ADAP2)
			return;
		else
			method=ADAP2;
		break;	
    }	
		
	this->method = method;
	
	// Load other set of shaders and update the result
	loadShaders();
	if (!init) {
		return;
	}
	
	updateSelection(boundaryVector, selectionPoly, false);
	
	update();
}
