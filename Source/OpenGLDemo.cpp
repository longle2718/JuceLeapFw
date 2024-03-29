/*
  ==============================================================================

   This file is part of the JUCE library - "Jules' Utility Class Extensions"
   Copyright 2004-12 by Raw Material Software Ltd.

  ------------------------------------------------------------------------------

   JUCE can be redistributed and/or modified under the terms of the GNU General
   Public License (Version 2), as published by the Free Software Foundation.
   A copy of the license is included in the JUCE distribution, or can be found
   online at www.gnu.org/licenses.

   JUCE is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
   A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

  ------------------------------------------------------------------------------

   To release a closed-source product which uses JUCE, commercial licenses are
   available: visit www.rawmaterialsoftware.com/juce for more information.

  ==============================================================================
*/

#include "DAQMXclass.h"
#include "JuceDemoHeader.h"
#include "WavefrontObjParser.h"
#include "Leap.h"
#include "LeapUtil.h"
#include "LeapUtilGL.h"
#include <cctype>

//==============================================================================
struct OpenGLDemoClasses
{
    /** Vertex data to be passed to the shaders.
        For the purposes of this demo, each vertex will have a 3D position, a colour and a
        2D texture co-ordinate. Of course you can ignore these or manipulate them in the
        shader programs but are some useful defaults to work from.
     */
    struct Vertex
    {
        float position[3];
        float normal[3];
        float colour[4];
        float texCoord[2];
    };

    //==============================================================================
    // This class just manages the attributes that the demo shaders use.
    struct Attributes
    {
        Attributes (OpenGLContext& openGLContext, OpenGLShaderProgram& shader)
        {
            position      = createAttribute (openGLContext, shader, "position");
            normal        = createAttribute (openGLContext, shader, "normal");
            sourceColour  = createAttribute (openGLContext, shader, "sourceColour");
            texureCoordIn = createAttribute (openGLContext, shader, "texureCoordIn");
        }

        void enable (OpenGLContext& openGLContext)
        {
            if (position != nullptr)
            {
                openGLContext.extensions.glVertexAttribPointer (position->attributeID, 3, GL_FLOAT, GL_FALSE, sizeof (Vertex), 0);
                openGLContext.extensions.glEnableVertexAttribArray (position->attributeID);
            }

            if (normal != nullptr)
            {
                openGLContext.extensions.glVertexAttribPointer (normal->attributeID, 3, GL_FLOAT, GL_FALSE, sizeof (Vertex), (GLvoid*) (sizeof (float) * 3));
                openGLContext.extensions.glEnableVertexAttribArray (normal->attributeID);
            }

            if (sourceColour != nullptr)
            {
                openGLContext.extensions.glVertexAttribPointer (sourceColour->attributeID, 4, GL_FLOAT, GL_FALSE, sizeof (Vertex), (GLvoid*) (sizeof (float) * 6));
                openGLContext.extensions.glEnableVertexAttribArray (sourceColour->attributeID);
            }

            if (texureCoordIn != nullptr)
            {
                openGLContext.extensions.glVertexAttribPointer (texureCoordIn->attributeID, 2, GL_FLOAT, GL_FALSE, sizeof (Vertex), (GLvoid*) (sizeof (float) * 10));
                openGLContext.extensions.glEnableVertexAttribArray (texureCoordIn->attributeID);
            }
        }

        void disable (OpenGLContext& openGLContext)
        {
            if (position != nullptr)       openGLContext.extensions.glDisableVertexAttribArray (position->attributeID);
            if (normal != nullptr)         openGLContext.extensions.glDisableVertexAttribArray (normal->attributeID);
            if (sourceColour != nullptr)   openGLContext.extensions.glDisableVertexAttribArray (sourceColour->attributeID);
            if (texureCoordIn != nullptr)  openGLContext.extensions.glDisableVertexAttribArray (texureCoordIn->attributeID);
        }

        ScopedPointer<OpenGLShaderProgram::Attribute> position, normal, sourceColour, texureCoordIn;

    private:
        static OpenGLShaderProgram::Attribute* createAttribute (OpenGLContext& openGLContext,
                                                                OpenGLShaderProgram& shader,
                                                                const char* attributeName)
        {
            if (openGLContext.extensions.glGetAttribLocation (shader.programID, attributeName) < 0)
                return nullptr;

            return new OpenGLShaderProgram::Attribute (shader, attributeName);
        }
    };

    //==============================================================================
    // This class just manages the uniform values that the demo shaders use.
    struct Uniforms
    {
        Uniforms (OpenGLContext& openGLContext, OpenGLShaderProgram& shader)
        {
            projectionMatrix = createUniform (openGLContext, shader, "projectionMatrix");
            viewMatrix       = createUniform (openGLContext, shader, "viewMatrix");
            texture          = createUniform (openGLContext, shader, "texture");
            lightPosition    = createUniform (openGLContext, shader, "lightPosition");
            bouncingNumber   = createUniform (openGLContext, shader, "bouncingNumber");
        }

        ScopedPointer<OpenGLShaderProgram::Uniform> projectionMatrix, viewMatrix, texture, lightPosition, bouncingNumber;

    private:
        static OpenGLShaderProgram::Uniform* createUniform (OpenGLContext& openGLContext,
                                                            OpenGLShaderProgram& shader,
                                                            const char* uniformName)
        {
            if (openGLContext.extensions.glGetUniformLocation (shader.programID, uniformName) < 0)
                return nullptr;

            return new OpenGLShaderProgram::Uniform (shader, uniformName);
        }
    };

    //==============================================================================
    /** This loads a 3D model from an OBJ file and converts it into some vertex buffers
        that we can draw.
    */
    struct Shape
    {
        Shape (OpenGLContext& openGLContext)
        {
            if (shapeFile.load (BinaryData::teapot_obj).wasOk())
                for (int i = 0; i < shapeFile.shapes.size(); ++i)
                    vertexBuffers.add (new VertexBuffer (openGLContext, *shapeFile.shapes.getUnchecked(i)));

        }

        void draw (OpenGLContext& openGLContext, Attributes& attributes)
        {
            for (int i = 0; i < vertexBuffers.size(); ++i)
            {
                VertexBuffer& vertexBuffer = *vertexBuffers.getUnchecked (i);
                vertexBuffer.bind();

                attributes.enable (openGLContext);
                glDrawElements (GL_TRIANGLES, vertexBuffer.numIndices, GL_UNSIGNED_INT, 0);
                attributes.disable (openGLContext);
            }
        }

    private:
        struct VertexBuffer
        {
            VertexBuffer (OpenGLContext& context, WavefrontObjFile::Shape& shape) : openGLContext (context)
            {
                numIndices = shape.mesh.indices.size();

                openGLContext.extensions.glGenBuffers (1, &vertexBuffer);
                openGLContext.extensions.glBindBuffer (GL_ARRAY_BUFFER, vertexBuffer);

                Array<Vertex> vertices;
                createVertexListFromMesh (shape.mesh, vertices, Colours::green);

                openGLContext.extensions.glBufferData (GL_ARRAY_BUFFER, vertices.size() * sizeof (Vertex),
                                                       vertices.getRawDataPointer(), GL_STATIC_DRAW);

                openGLContext.extensions.glGenBuffers (1, &indexBuffer);
                openGLContext.extensions.glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
                openGLContext.extensions.glBufferData (GL_ELEMENT_ARRAY_BUFFER, numIndices * sizeof (juce::uint32),
                                                       shape.mesh.indices.getRawDataPointer(), GL_STATIC_DRAW);
            }

            ~VertexBuffer()
            {
                openGLContext.extensions.glDeleteBuffers (1, &vertexBuffer);
                openGLContext.extensions.glDeleteBuffers (1, &indexBuffer);
            }

            void bind()
            {
                openGLContext.extensions.glBindBuffer (GL_ARRAY_BUFFER, vertexBuffer);
                openGLContext.extensions.glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
            }

            GLuint vertexBuffer, indexBuffer;
            int numIndices;
            OpenGLContext& openGLContext;

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VertexBuffer)
        };

        WavefrontObjFile shapeFile;
        OwnedArray<VertexBuffer> vertexBuffers;

        static void createVertexListFromMesh (const WavefrontObjFile::Mesh& mesh, Array<Vertex>& list, Colour colour)
        {
            const float scale = 0.2f;
            WavefrontObjFile::TextureCoord defaultTexCoord = { 0.5f, 0.5f };
            WavefrontObjFile::Vertex defaultNormal = { 0.5f, 0.5f, 0.5f };

            for (int i = 0; i < mesh.vertices.size(); ++i)
            {
                const WavefrontObjFile::Vertex& v = mesh.vertices.getReference (i);

                const WavefrontObjFile::Vertex& n
                        = i < mesh.normals.size() ? mesh.normals.getReference (i) : defaultNormal;

                const WavefrontObjFile::TextureCoord& tc
                        = i < mesh.textureCoords.size() ? mesh.textureCoords.getReference (i) : defaultTexCoord;

                Vertex vert =
                {
                    { scale * v.x, scale * v.y, scale * v.z, },
                    { scale * n.x, scale * n.y, scale * n.z, },
                    { colour.getFloatRed(), colour.getFloatGreen(), colour.getFloatBlue(), colour.getFloatAlpha() },
                    { tc.x, tc.y }
                };

                list.add (vert);
            }
        }
    };

    //==============================================================================
    // These classes are used to load textures from the various sources that the demo uses..
    struct DemoTexture
    {
        virtual ~DemoTexture() {}
        virtual bool applyTo (OpenGLTexture&) = 0;

        String name;
    };

    struct DynamicTexture   : public DemoTexture
    {
        DynamicTexture() { name = "Dynamically-generated texture"; }

        Image image;
        BouncingNumber x, y;

        bool applyTo (OpenGLTexture& texture) override
        {
            const int size = 128;

            if (! image.isValid())
                image = Image (Image::ARGB, size, size, true);

            {
                Graphics g (image);
                g.fillAll (Colours::lightcyan);

                g.setColour (Colours::darkred);
                g.drawRect (0, 0, size, size, 2);

                g.setColour (Colours::green);
                g.fillEllipse (x.getValue() * size * 0.9f, y.getValue() * size * 0.9f, size * 0.1f, size * 0.1f);

                g.setColour (Colours::black);
                g.setFont (40);
                g.drawFittedText (String (Time::getCurrentTime().getMilliseconds()), image.getBounds(), Justification::centred, 1);
            }

            texture.loadImage (image);
            return true;
        }
    };

    struct BuiltInTexture   : public DemoTexture
    {
        BuiltInTexture (const char* nm, const void* imageData, int imageSize)
            : image (resizeImageToPowerOfTwo (ImageFileFormat::loadFrom (imageData, imageSize)))
        {
            name = nm;
        }

        Image image;

        bool applyTo (OpenGLTexture& texture) override
        {
            texture.loadImage (image);
            return false;
        }
    };

    struct TextureFromFile   : public DemoTexture
    {
        TextureFromFile (const File& file)
        {
            name = file.getFileName();
            image = resizeImageToPowerOfTwo (ImageFileFormat::loadFrom (file));
        }

        Image image;

        bool applyTo (OpenGLTexture& texture) override
        {
            texture.loadImage (image);
            return false;
        }
    };

    static Image resizeImageToPowerOfTwo (Image image)
    {
        if (! (isPowerOfTwo (image.getWidth()) && isPowerOfTwo (image.getHeight())))
            return image.rescaled (jmin (1024, nextPowerOfTwo (image.getWidth())),
                                   jmin (1024, nextPowerOfTwo (image.getHeight())));

        return image;
    }

    class OpenGLDemo;

    //==============================================================================
    /**
        This component sits on top of the main GL demo, and contains all the sliders
        and widgets that control things.
    */
    class DemoControlsOverlay  : public Component,
                                 private CodeDocument::Listener,
                                 private ComboBox::Listener,
                                 private Slider::Listener,
                                 private Button::Listener,
                                 private Timer
    {
    public:
        DemoControlsOverlay (OpenGLDemo& d)
            : demo (d),
              vertexEditorComp (vertexDocument, nullptr),
              fragmentEditorComp (fragmentDocument, nullptr),
              tabbedComp (TabbedButtonBar::TabsAtLeft),
              showBackgroundToggle ("Draw 2D graphics in background")
        {
            addAndMakeVisible (statusLabel);
            statusLabel.setJustificationType (Justification::topLeft);
            statusLabel.setColour (Label::textColourId, Colours::black);
            statusLabel.setFont (Font (14.0f));

            //addAndMakeVisible (sizeSlider);
            //sizeSlider.setRange (0.0, 1.0, 0.001);
            //sizeSlider.addListener (this);

            //addAndMakeVisible (sizeLabel);
            //sizeLabel.setText ("Size:", dontSendNotification);
            //sizeLabel.attachToComponent (&sizeSlider, true);

            addAndMakeVisible (speedSlider);
            speedSlider.setRange (0.0, 0.5, 0.001);
            speedSlider.addListener (this);
            speedSlider.setSkewFactor (0.5f);

            addAndMakeVisible (speedLabel);
            speedLabel.setText ("Speed:", dontSendNotification);
            speedLabel.attachToComponent (&speedSlider, true);

            addAndMakeVisible (showBackgroundToggle);
            showBackgroundToggle.addListener (this);

            Colour editorBackground (Colours::white.withAlpha (0.6f));

            addAndMakeVisible (tabbedComp);
            tabbedComp.setTabBarDepth (25);
            tabbedComp.setColour (TabbedButtonBar::tabTextColourId, Colours::grey);
            tabbedComp.addTab ("Vertex", editorBackground, &vertexEditorComp, false);
            tabbedComp.addTab ("Fragment", editorBackground, &fragmentEditorComp, false);

            vertexEditorComp.setColour (CodeEditorComponent::backgroundColourId, editorBackground);
            fragmentEditorComp.setColour (CodeEditorComponent::backgroundColourId, editorBackground);

            vertexDocument.addListener (this);
            fragmentDocument.addListener (this);

            textures.add (new BuiltInTexture ("Portmeirion", BinaryData::portmeirion_jpg, BinaryData::portmeirion_jpgSize));
            textures.add (new BuiltInTexture ("Brushed aluminium", BinaryData::brushed_aluminium_png, BinaryData::brushed_aluminium_pngSize));
            textures.add (new BuiltInTexture ("JUCE logo", BinaryData::juce_icon_png, BinaryData::juce_icon_pngSize));
            textures.add (new DynamicTexture());

            addAndMakeVisible (textureBox);
            textureBox.addListener (this);
            updateTexturesList();

            addAndMakeVisible (presetBox);
            presetBox.addListener (this);
            Array<ShaderPreset> presets (getPresets());
            StringArray presetNames;

            for (int i = 0; i < presets.size(); ++i)
                presetBox.addItem (presets[i].name, i + 1);

            addAndMakeVisible (presetLabel);
            presetLabel.setText ("Shader Preset:", dontSendNotification);
            presetLabel.attachToComponent (&presetBox, true);
			
            addAndMakeVisible (textureLabel);
            textureLabel.setText ("Texture:", dontSendNotification);
            textureLabel.attachToComponent (&textureBox, true);
        }

        void initialise()
        {
            showBackgroundToggle.setToggleState (false, sendNotification);
            textureBox.setSelectedItemIndex (0);
            presetBox.setSelectedItemIndex (0);
            speedSlider.setValue (0.01);
            //sizeSlider.setValue (0.5);
        }

        void resized() override
        {
            juce::Rectangle<int> area (getLocalBounds().reduced (4));

            juce::Rectangle<int> top (area.removeFromTop (75));

            juce::Rectangle<int> sliders (top.removeFromRight (area.getWidth() / 2));
            showBackgroundToggle.setBounds (sliders.removeFromBottom (25));
            speedSlider.setBounds (sliders.removeFromBottom (25));
            //sizeSlider.setBounds (sliders.removeFromBottom (25));

            top.removeFromRight (70);
            statusLabel.setBounds (top);

            juce::Rectangle<int> shaderArea (area.removeFromBottom (area.getHeight() / 8));
            juce::Rectangle<int> presets (shaderArea.removeFromTop (25));
            presets.removeFromLeft (100);
            presetBox.setBounds (presets.removeFromLeft (150));
            presets.removeFromLeft (100);
            textureBox.setBounds (presets);

            shaderArea.removeFromTop (4);
            tabbedComp.setBounds (shaderArea);
        }

        void mouseDown (const MouseEvent& e) override
        {
            //demo.draggableOrientation.mouseDown (e.getPosition());
			demo.camera.OnMouseDown( LeapUtil::FromVector2( e.getPosition() ) );
        }

        void mouseDrag (const MouseEvent& e) override
        {
            //demo.draggableOrientation.mouseDrag (e.getPosition());
			demo.camera.OnMouseMoveOrbit( LeapUtil::FromVector2( e.getPosition() ) );
        }

        void mouseWheelMove (const MouseEvent&, const MouseWheelDetails& d) override
        {
            //sizeSlider.setValue (sizeSlider.getValue() + d.deltaY);
			demo.camera.OnMouseWheel( d.deltaY );
        }

        void mouseMagnify (const MouseEvent&, float magnifyAmmount) override
        {
			(void) magnifyAmmount;
            //sizeSlider.setValue (sizeSlider.getValue() + magnifyAmmount - 1.0f);
        }

        void selectPreset (int preset)
        {
            const ShaderPreset& p = getPresets()[preset];

            vertexDocument.replaceAllContent (p.vertexShader);
            fragmentDocument.replaceAllContent (p.fragmentShader);

            startTimer (1);
        }

        void selectTexture (int itemID)
        {
           #if JUCE_MODAL_LOOPS_PERMITTED
            if (itemID == 1000)
            {
                static File lastLocation = File::getSpecialLocation (File::userPicturesDirectory);

                FileChooser fc ("Choose an image to open...", lastLocation, "*.jpg;*.jpeg;*.png;*.gif");

                if (fc.browseForFileToOpen())
                {
                    lastLocation = fc.getResult();

                    textures.add (new TextureFromFile (fc.getResult()));
                    updateTexturesList();

                    textureBox.setSelectedId (textures.size());
                }
            }
            else
           #endif
            {
                if (DemoTexture* t = textures [itemID - 1])
                    demo.setTexture (t);
            }
        }

        void updateTexturesList()
        {
            textureBox.clear();

            for (int i = 0; i < textures.size(); ++i)
                textureBox.addItem (textures.getUnchecked(i)->name, i + 1);

           #if JUCE_MODAL_LOOPS_PERMITTED
            textureBox.addSeparator();
            textureBox.addItem ("Load from a file...", 1000);
           #endif
        }

        Label statusLabel;

    private:
        void sliderValueChanged (Slider*) override
        {
            //demo.scale = (float) sizeSlider.getValue();
            demo.rotationSpeed = (float) speedSlider.getValue();
        }

        void buttonClicked (Button*)
        {
            demo.doBackgroundDrawing = showBackgroundToggle.getToggleState();
        }

        enum { shaderLinkDelay = 500 };

        void codeDocumentTextInserted (const String& /*newText*/, int /*insertIndex*/) override
        {
            startTimer (shaderLinkDelay);
        }

        void codeDocumentTextDeleted (int /*startIndex*/, int /*endIndex*/) override
        {
            startTimer (shaderLinkDelay);
        }

        void timerCallback() override
        {
            stopTimer();
            demo.setShaderProgram (vertexDocument.getAllContent(),
                                   fragmentDocument.getAllContent());
        }

        void comboBoxChanged (ComboBox* box) override
        {
            if (box == &presetBox)
                selectPreset (presetBox.getSelectedItemIndex());
            else if (box == &textureBox)
                selectTexture (textureBox.getSelectedId());
        }

        OpenGLDemo& demo;

        Label speedLabel;//, sizeLabel;
		Slider speedSlider;//, sizeSlider;

        CodeDocument vertexDocument, fragmentDocument;
        CodeEditorComponent vertexEditorComp, fragmentEditorComp;
        TabbedComponent tabbedComp;

        ComboBox presetBox, textureBox;
        Label presetLabel, textureLabel;

        ToggleButton showBackgroundToggle;

        OwnedArray<DemoTexture> textures;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DemoControlsOverlay)

    };

    //==============================================================================
    /** This is the main demo component - the GL context gets attached to it, and
        it implements the OpenGLRenderer callback so that it can do real GL work.
    */
    class OpenGLDemo  : public Component,
                        private OpenGLRenderer,
						Leap::Listener,
						CameraDevice::Listener
    {
    public:
        OpenGLDemo()
            : doBackgroundDrawing (false),
              scale (0.5f), rotationSpeed (0.0f), rotation (0.0f),
              textureToUse (nullptr)
        {
            MainAppWindow::getMainAppWindow()->setRenderingEngine (0);

            setOpaque (true);
            addAndMakeVisible (controlsOverlay = new DemoControlsOverlay (*this));

            openGLContext.setRenderer (this);
            openGLContext.attachTo (*this);
            openGLContext.setContinuousRepainting (false);

            controlsOverlay->initialise();

			OpenGLDemoClasses::getController().addListener( *this );
			initColors();
			resetCamera();
			
			m_fFrameScale = 0.005f;
			m_mtxFrameTransform.origin = Leap::Vector( 0.0f, -1.0f, 0.125f );
			m_fPointableRadius = 0.025f;
			SP = new DAQMX("Dev1/port3/line0:7");
			StringArray camDevList = CameraDevice::getAvailableDevices();
			camDevPtr = CameraDevice::openDevice(0);
			if ( camDevPtr != nullptr)
			{
				camDevPtr->addListener( this );
			}
        }

        ~OpenGLDemo()
        {
			OpenGLDemoClasses::getController().removeListener( *this );

			unsigned char tmp[8] = {0,0,0,0,0,0,0,0};
			SP->writePWM(tmp);
			delete SP;

            openGLContext.detach();
			
			OpenGLDemoClasses::getController().removeListener( *this );
			
			if ( camDevPtr != nullptr)
			{
				camDevPtr->removeListener( this );
				camDevPtr->~CameraDevice();
			}
			camDevPtr = nullptr;
        }

        void newOpenGLContextCreated() override
        {
            // nothing to do in this case - we'll initialise our shaders + textures
            // on demand, during the render callback.
        }

        void openGLContextClosing() override
        {
            // When the context is about to close, you must use this callback to delete
            // any GPU resources while the context is still current.
            shape = nullptr;
            shader = nullptr;
            attributes = nullptr;
            uniforms = nullptr;
            texture.release();
        }

        // This is a virtual method in OpenGLRenderer, and is called when it's time
        // to do your GL rendering.
        void renderOpenGL() override
        {
            jassert (OpenGLHelpers::isContextActive());

            const float desktopScale = (float) openGLContext.getRenderingScale();
            OpenGLHelpers::clear (Colours::lightblue);

            if (textureToUse != nullptr)
                if (! textureToUse->applyTo (texture))
                    textureToUse = nullptr;

            // First draw our background graphics to demonstrate the OpenGLGraphicsContext class
            if (doBackgroundDrawing)
                drawBackground2DStuff (desktopScale);

            // Having used the juce 2D renderer, it will have messed-up a whole load of GL state, so
            // we need to initialise some important settings before doing our normal GL 3D drawing..
            glEnable (GL_DEPTH_TEST);
            glDepthFunc (GL_LESS);
            glEnable (GL_BLEND);
            glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            openGLContext.extensions.glActiveTexture (GL_TEXTURE0);
            glEnable (GL_TEXTURE_2D);

			LeapUtilGL::GLMatrixScope sceneMatrixScope;
			setupScene();

			// Draw the Leap frame
			Leap::Frame frame = m_lastFrame;
			drawLeapFrame(frame);

			/*
			updateShader();   // Check whether we need to compile a new shader

            if (shader == nullptr)
                return;

            glViewport (0, 0, roundToInt (desktopScale * getWidth()), roundToInt (desktopScale * getHeight()));

            texture.bind();

            glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

            shader->use();

            if (uniforms->projectionMatrix != nullptr)
                uniforms->projectionMatrix->setMatrix4 (getProjectionMatrix().mat, 1, false);

            if (uniforms->viewMatrix != nullptr)
                uniforms->viewMatrix->setMatrix4 (getViewMatrix().mat, 1, false);

            if (uniforms->texture != nullptr)
                uniforms->texture->set ((GLint) 0);

            if (uniforms->lightPosition != nullptr)
                uniforms->lightPosition->set (-15.0f, 10.0f, 15.0f, 0.0f);

            if (uniforms->bouncingNumber != nullptr)
                uniforms->bouncingNumber->set (bouncingNumber.getValue());

            shape->draw (openGLContext, *attributes);

            // Reset the element buffers so child Components draw correctly
            openGLContext.extensions.glBindBuffer (GL_ARRAY_BUFFER, 0);
            openGLContext.extensions.glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, 0);

            if (! controlsOverlay->isMouseButtonDown())
                rotation += (float) rotationSpeed;
			*/
        }

		/*
        Matrix3D<float> getProjectionMatrix() const
        {
            float w = 1.0f / (scale + 0.1f);
            float h = w * getLocalBounds().toFloat().getAspectRatio (false);
            return Matrix3D<float>::fromFrustum (-w, w, -h, h, 4.0f, 30.0f);
        }

        Matrix3D<float> getViewMatrix() const
        {
            Matrix3D<float> viewMatrix (Vector3D<float> (0.0f, 1.0f, -10.0f));

            viewMatrix *= draggableOrientation.getRotationMatrix();

            Matrix3D<float> rotationMatrix = viewMatrix.rotated (Vector3D<float> (rotation, rotation, -0.3f));

            return viewMatrix * rotationMatrix;
        }
		*/
        void setTexture (DemoTexture* t)
        {
            textureToUse = t;
        }

        void setShaderProgram (const String& vertexShader, const String& fragmentShader)
        {
            newVertexShader = vertexShader;
            newFragmentShader = fragmentShader;
        }

        void paint (Graphics&) {}

        void resized() override
        {
            controlsOverlay->setBounds (getLocalBounds());
            draggableOrientation.setViewport (getLocalBounds());
        }

		void initColors()
		{
		  float fMin      = 0.0f;
		  float fMax      = 1.0f;
		  float fNumSteps = static_cast<float>(pow( kNumColors, 1.0/3.0 ));
		  float fStepSize = (fMax - fMin)/fNumSteps;
		  float fR = fMin, fG = fMin, fB = fMin;

		  for ( uint32_t i = 0; i < kNumColors; i++ )
		  {
			m_avColors[i] = Leap::Vector( fR, fG, LeapUtil::Min(fB, fMax) );

			fR += fStepSize;

			if ( fR > fMax )
			{
			  fR = fMin;
			  fG += fStepSize;

			  if ( fG > fMax )
			  {
				fG = fMin;
				fB += fStepSize;
			  }
			}
		  }

		  Random rng(0x13491349);

		  for ( uint32_t i = 0; i < kNumColors; i++ )
		  {
			uint32_t      uiRandIdx    = i + (rng.nextInt() % (kNumColors - i));
			Leap::Vector  vSwapTemp    = m_avColors[i];

			m_avColors[i]          = m_avColors[uiRandIdx];
			m_avColors[uiRandIdx]  = vSwapTemp;
		  }
		}

		void resetCamera()
		{
			camera.SetOrbitTarget( Leap::Vector::zero() );
			camera.SetPOVLookAt( Leap::Vector( 0, 2, 4 ), camera.GetOrbitTarget() );
		}

		/// affects model view matrix.  needs to be inside a glPush/glPop matrix block!
		void setupScene()
		{
			camera.SetAspectRatio( getWidth() / static_cast<float>(getHeight()) );

			camera.SetupGLProjection();

			camera.ResetGLView();

			camera.SetupGLView();
		}

		void onFrame(const Leap::Controller& controller) override
		{
			Leap::Frame frame = controller.frame();
			m_lastFrame = frame;
			openGLContext.triggerRepaint();
		}

		void imageReceived(const Image &image) override
		{
			m_lastImage = image;
			openGLContext.triggerRepaint();
		}

		void drawLeapFrame( Leap::Frame frame )
		{
			LeapUtilGL::GLAttribScope colorScope( GL_CURRENT_BIT | GL_LINE_BIT );
			glLineWidth( 3.0f );

			const float fScale = m_fPointableRadius;			
			const Leap::HandList& hands = frame.hands();
			Colour leftClr(Colours::white), rightClr(Colours::white), 
				upClr(Colours::white), downClr(Colours::white);

			for (size_t j = 0, m = hands.count(); j < m; j++)
			{
				const Leap::Hand& hand = hands[j];
				Leap::Vector palmPos = m_mtxFrameTransform.transformPoint( hand.palmPosition() * m_fFrameScale );
				Leap::Vector palmNor = m_mtxFrameTransform.transformDirection( hand.palmNormal() );

				LeapUtilGL::drawDisk( palmPos, palmNor );

				const Leap::PointableList& pointables = hand.pointables();

				// create variable for duty cycles: ch1, skip, ch2, ch3, ch4, ...
				unsigned char dc[8] = {0,0,0,0,0,0,0,0};
				float val = 0;

				for ( size_t i = 0, n = pointables.count(); i < n; i++ )
				{
					const Leap::Pointable&  pointable   = pointables[i];
					Leap::Vector            vStartPos   = m_mtxFrameTransform.transformPoint( pointable.tipPosition() * m_fFrameScale );
					Leap::Vector            vEndPos     = m_mtxFrameTransform.transformDirection( pointable.direction() ) * -0.125f;

					glColor3f( 1, 0, 0 );

					{
						LeapUtilGL::GLMatrixScope matrixScope;
						
						glTranslatef( vStartPos.x, vStartPos.y, vStartPos.z );

						glBegin(GL_LINES);

						glVertex3f( 0, 0, 0 );
						glVertex3fv( vEndPos.toFloatPointer() );
						glVertex3fv( vEndPos.toFloatPointer() );
						glVertex3fv( (palmPos-vStartPos).toFloatPointer() );

						glEnd();

						glScalef( fScale, fScale, fScale );

						LeapUtilGL::drawSphere( LeapUtilGL::kStyle_Solid );
					}

					// Check for intersection with the cylinders
					if (vStartPos.distanceTo(Leap::Vector(vStartPos.x, vStartPos.y, 0)) <= 0.1)
					{
						if (vStartPos.y>=0.4 && vStartPos.y<=0.6 && vStartPos.x>=-0.5 && vStartPos.x<=0.5)
						{
							upClr = Colours::red;
							// determine value
							val = int(255*(vStartPos.x/2+0.75));
							// vibrate M1 (p3.0) and M2 (p3.2)
							dc[0] = val;
							dc[2] = 383-val;
						}
						if (vStartPos.y>=-0.6 && vStartPos.y<=-0.4 && vStartPos.x>=-0.5 && vStartPos.x<=0.5)
						{
							downClr = Colours::red;
							// determine value
							val = int(255*(vStartPos.x/2+0.75));
							// vibrate M3 (p3.3) and M4 (p3.4)
							dc[3] = 383-val;
							dc[4] = val;
						}
						if (vStartPos.y>=-0.5 && vStartPos.y<=0.5 && vStartPos.x>=0.4 && vStartPos.x<=0.6)
						{
							rightClr = Colours::red;
							// determine value
							val = int(255*(vStartPos.y/2+0.75));
							// vibrate M1 and M4
							dc[0] = val;
							dc[4] = 383-val;
						}
						if (vStartPos.y>=-0.5 && vStartPos.y<=0.5 && vStartPos.x>=-0.6 && vStartPos.x<=-0.4)
						{
							leftClr = Colours::red;
							// determine value
							val = int(255*(vStartPos.y/2+0.75));
							// vibrate M2 and M3
							dc[2] = val;
							dc[3] = 383-val;
						}
					}
				}
				// write new DC to DAQmx
				SP->writePWM(dc);

			}

			// Draw the region of interest
			{
				LeapUtilGL::GLMatrixScope roiMatrixScope;

				glTranslatef(0.5, 0, 0);
				drawCylinder(LeapUtilGL::kStyle_Solid, LeapUtilGL::kAxis_Y, 0.1f, 1.0f, rightClr);
				glTranslatef(-1, 0, 0);
				drawCylinder(LeapUtilGL::kStyle_Solid, LeapUtilGL::kAxis_Y, 0.1f, 1.0f, leftClr);
				glTranslatef(0.5, 0.5, 0);
				drawCylinder(LeapUtilGL::kStyle_Solid, LeapUtilGL::kAxis_X, 0.1f, 1.0f, upClr);
				glTranslatef(0, -1, 0);
				drawCylinder(LeapUtilGL::kStyle_Solid, LeapUtilGL::kAxis_X, 0.1f, 1.0f, downClr);
			}
		}

		Draggable3DOrientation draggableOrientation;
        bool doBackgroundDrawing;
        float scale, rotationSpeed;
        BouncingNumber bouncingNumber;
		LeapUtilGL::CameraGL        camera;

    private:
		Leap::Frame                 m_lastFrame;
		enum  { kNumColors = 256 };
		Leap::Vector				m_avColors[kNumColors];
		float                       m_fPointableRadius;
		Leap::Matrix                m_mtxFrameTransform;
		float                       m_fFrameScale;
		DAQMX*						SP;
		CameraDevice*				camDevPtr;
		Image						m_lastImage;

        OpenGLContext openGLContext;

        ScopedPointer<DemoControlsOverlay> controlsOverlay;

        float rotation;

        ScopedPointer<OpenGLShaderProgram> shader;
        ScopedPointer<Shape> shape;
        ScopedPointer<Attributes> attributes;
        ScopedPointer<Uniforms> uniforms;

        OpenGLTexture texture;
        DemoTexture* textureToUse;

        String newVertexShader, newFragmentShader;

		/*
        struct BackgroundStar
        {
            SlowerBouncingNumber x, y, hue, angle;
        };

        BackgroundStar stars[3];
		*/

        void updateShader()
        {
            if (newVertexShader.isNotEmpty() || newFragmentShader.isNotEmpty())
            {
                ScopedPointer<OpenGLShaderProgram> newShader (new OpenGLShaderProgram (openGLContext));
                String statusText;

                if (newShader->addVertexShader (newVertexShader)
                      && newShader->addFragmentShader (newFragmentShader)
                      && newShader->link())
                {
                    shape = nullptr;
                    attributes = nullptr;
                    uniforms = nullptr;

                    shader = newShader;
                    shader->use();

                    shape      = new Shape (openGLContext);
                    attributes = new Attributes (openGLContext, *shader);
                    uniforms   = new Uniforms (openGLContext, *shader);

                   #if ! JUCE_OPENGL_ES
                    statusText = "GLSL: v" + String (OpenGLShaderProgram::getLanguageVersion(), 2);
                   #else
                    statusText = "GLSL ES";
                   #endif
                }
                else
                {
                    statusText = newShader->getLastError();
                }

                controlsOverlay->statusLabel.setText (statusText, dontSendNotification);


                newVertexShader = String::empty;
                newFragmentShader = String::empty;
            }
        }

		void drawBackground2DStuff (float desktopScale)
        {
            // Create an OpenGLGraphicsContext that will draw into this GL window..
            ScopedPointer<LowLevelGraphicsContext> glRenderer (createOpenGLGraphicsContext (openGLContext,
                                                                                            roundToInt (desktopScale * getWidth()),
                                                                                            roundToInt (desktopScale * getHeight())));

            if (glRenderer != nullptr)
            {
                Graphics g (*glRenderer);
				//Image bg = ImageFileFormat::loadFrom (BinaryData::portmeirion_jpg, BinaryData::portmeirion_jpgSize);
				
                //g.addTransform (AffineTransform::scale (desktopScale*getWidth()/bg.getWidth(), 
				//	desktopScale*getHeight()/bg.getHeight()));
				g.addTransform (AffineTransform::scale (desktopScale*getWidth()/m_lastImage.getWidth(), 
					desktopScale*getHeight()/m_lastImage.getHeight()));
				g.drawImageAt( m_lastImage, 0, 0 );
				
				/*
                for (int i = 0; i < numElementsInArray (stars); ++i)
                {
                    float size = 0.25f;

                    // This stuff just creates a spinning star shape and fills it..
                    Path p;
                    p.addStar (Point<float> (getWidth() * stars[i].x.getValue(),
                                             getHeight() * stars[i].y.getValue()), 7,
                               getHeight() * size * 0.5f,
                               getHeight() * size,
                               stars[i].angle.getValue());

                    float hue = stars[i].hue.getValue();

                    g.setGradientFill (ColourGradient (Colours::green.withRotatedHue (hue).withAlpha (0.8f),
                                                       0, 0,
                                                       Colours::red.withRotatedHue (hue).withAlpha (0.5f),
                                                       0, (float) getHeight(), false));
                    g.fillPath (p);
                }
				*/
            }
        }

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OpenGLDemo)
    };

    //==============================================================================
    struct ShaderPreset
    {
        const char* name;
        const char* vertexShader;
        const char* fragmentShader;
    };
	
	static Array<ShaderPreset> getPresets()
    {
        #define SHADER_DEMO_HEADER \
            "//  This is a live OpenGL Shader demo.\n" \
            "//  Edit the shader program below and it will be \n" \
            "//  compiled and applied to the model above!\n" \
            "//\n\n"

        ShaderPreset presets[] =
        {
            {
                "Texture + Lighting",

                SHADER_DEMO_HEADER
                "attribute vec4 position;\n"
                "attribute vec4 normal;\n"
                "attribute vec4 sourceColour;\n"
                "attribute vec2 texureCoordIn;\n"
                "\n"
                "uniform mat4 projectionMatrix;\n"
                "uniform mat4 viewMatrix;\n"
                "uniform vec4 lightPosition;\n"
                "\n"
                "varying vec4 destinationColour;\n"
                "varying vec2 textureCoordOut;\n"
                "varying float lightIntensity;\n"
                "\n"
                "void main (void)\n"
                "{\n"
                "    destinationColour = sourceColour;\n"
                "    textureCoordOut = texureCoordIn;\n"
                "\n"
                "    vec4 light = viewMatrix * lightPosition;\n"
                "    lightIntensity = dot (light, normal);\n"
                "\n"
                "    gl_Position = projectionMatrix * viewMatrix * position;\n"
                "}\n",

                SHADER_DEMO_HEADER
               #if JUCE_OPENGL_ES
                "varying lowp vec4 destinationColour;\n"
                "varying lowp vec2 textureCoordOut;\n"
                "varying highp float lightIntensity;\n"
               #else
                "varying vec4 destinationColour;\n"
                "varying vec2 textureCoordOut;\n"
                "varying float lightIntensity;\n"
               #endif
                "\n"
                "uniform sampler2D texture;\n"
                "\n"
                "void main (void)\n"
                "{\n"
               #if JUCE_OPENGL_ES
                "   highp float l = max (0.3, lightIntensity * 0.3);\n"
                "   highp vec4 colour = vec4 (l, l, l, 1.0);\n"
               #else
                "   float l = max (0.3, lightIntensity * 0.3);\n"
                "   vec4 colour = vec4 (l, l, l, 1.0);\n"
               #endif
                "    gl_FragColor = colour * texture2D (texture, textureCoordOut);\n"
                "}\n"
            },

            {
                "Textured",

                SHADER_DEMO_HEADER
                "attribute vec4 position;\n"
                "attribute vec4 sourceColour;\n"
                "attribute vec2 texureCoordIn;\n"
                "\n"
                "uniform mat4 projectionMatrix;\n"
                "uniform mat4 viewMatrix;\n"
                "\n"
                "varying vec4 destinationColour;\n"
                "varying vec2 textureCoordOut;\n"
                "\n"
                "void main (void)\n"
                "{\n"
                "    destinationColour = sourceColour;\n"
                "    textureCoordOut = texureCoordIn;\n"
                "    gl_Position = projectionMatrix * viewMatrix * position;\n"
                "}\n",

                SHADER_DEMO_HEADER
               #if JUCE_OPENGL_ES
                "varying lowp vec4 destinationColour;\n"
                "varying lowp vec2 textureCoordOut;\n"
               #else
                "varying vec4 destinationColour;\n"
                "varying vec2 textureCoordOut;\n"
               #endif
                "\n"
                "uniform sampler2D texture;\n"
                "\n"
                "void main (void)\n"
                "{\n"
                "    gl_FragColor = texture2D (texture, textureCoordOut);\n"
                "}\n"
            },

            {
                "Flat Colour",

                SHADER_DEMO_HEADER
                "attribute vec4 position;\n"
                "attribute vec4 sourceColour;\n"
                "attribute vec2 texureCoordIn;\n"
                "\n"
                "uniform mat4 projectionMatrix;\n"
                "uniform mat4 viewMatrix;\n"
                "\n"
                "varying vec4 destinationColour;\n"
                "varying vec2 textureCoordOut;\n"
                "\n"
                "void main (void)\n"
                "{\n"
                "    destinationColour = sourceColour;\n"
                "    textureCoordOut = texureCoordIn;\n"
                "    gl_Position = projectionMatrix * viewMatrix * position;\n"
                "}\n",

                SHADER_DEMO_HEADER
               #if JUCE_OPENGL_ES
                "varying lowp vec4 destinationColour;\n"
                "varying lowp vec2 textureCoordOut;\n"
               #else
                "varying vec4 destinationColour;\n"
                "varying vec2 textureCoordOut;\n"
               #endif
                "\n"
                "uniform sampler2D texture;\n"
                "\n"
                "void main (void)\n"
                "{\n"
                "    gl_FragColor = destinationColour;\n"
                "}\n"
            },

            {
                "Rainbow",

                SHADER_DEMO_HEADER
                "attribute vec4 position;\n"
                "attribute vec4 sourceColour;\n"
                "attribute vec2 texureCoordIn;\n"
                "\n"
                "uniform mat4 projectionMatrix;\n"
                "uniform mat4 viewMatrix;\n"
                "\n"
                "varying vec4 destinationColour;\n"
                "varying vec2 textureCoordOut;\n"
                "\n"
                "varying float xPos;\n"
                "varying float yPos;\n"
                "varying float zPos;\n"
                "\n"
                "void main (void)\n"
                "{\n"
                "    vec4 v = vec4 (position);\n"
                "    xPos = clamp (v.x, 0.0, 1.0);\n"
                "    yPos = clamp (v.y, 0.0, 1.0);\n"
                "    zPos = clamp (v.z, 0.0, 1.0);\n"
                "    gl_Position = projectionMatrix * viewMatrix * position;\n"
                "}",

                SHADER_DEMO_HEADER
               #if JUCE_OPENGL_ES
                "varying lowp vec4 destinationColour;\n"
                "varying lowp vec2 textureCoordOut;\n"
                "varying lowp float xPos;\n"
                "varying lowp float yPos;\n"
                "varying lowp float zPos;\n"
               #else
                "varying vec4 destinationColour;\n"
                "varying vec2 textureCoordOut;\n"
                "varying float xPos;\n"
                "varying float yPos;\n"
                "varying float zPos;\n"
               #endif
                "\n"
                "void main (void)\n"
                "{\n"
                "    gl_FragColor = vec4 (xPos, yPos, zPos, 1.0);\n"
                "}"
            },

            {
                "Changing Colour",

                SHADER_DEMO_HEADER
                "attribute vec4 position;\n"
                "attribute vec2 texureCoordIn;\n"
                "\n"
                "uniform mat4 projectionMatrix;\n"
                "uniform mat4 viewMatrix;\n"
                "\n"
                "varying vec2 textureCoordOut;\n"
                "\n"
                "void main (void)\n"
                "{\n"
                "    textureCoordOut = texureCoordIn;\n"
                "    gl_Position = projectionMatrix * viewMatrix * position;\n"
                "}\n",

                SHADER_DEMO_HEADER
                "#define PI 3.1415926535897932384626433832795\n"
                "\n"
               #if JUCE_OPENGL_ES
                "precision mediump float;\n"
                "varying lowp vec2 textureCoordOut;\n"
               #else
                "varying vec2 textureCoordOut;\n"
               #endif
                "uniform float bouncingNumber;\n"
                "\n"
                "void main (void)\n"
                "{\n"
                "   float b = bouncingNumber;\n"
                "   float n = b * PI * 2.0;\n"
                "   float sn = (sin (n * textureCoordOut.x) * 0.5) + 0.5;\n"
                "   float cn = (sin (n * textureCoordOut.y) * 0.5) + 0.5;\n"
                "\n"
                "   vec4 col = vec4 (b, sn, cn, 1.0);\n"
                "   gl_FragColor = col;\n"
                "}\n"
            },

            {
                "Simple Light",

                SHADER_DEMO_HEADER
                "attribute vec4 position;\n"
                "attribute vec4 normal;\n"
                "\n"
                "uniform mat4 projectionMatrix;\n"
                "uniform mat4 viewMatrix;\n"
                "uniform vec4 lightPosition;\n"
                "\n"
                "varying float lightIntensity;\n"
                "\n"
                "void main (void)\n"
                "{\n"
                "    vec4 light = viewMatrix * lightPosition;\n"
                "    lightIntensity = dot (light, normal);\n"
                "\n"
                "    gl_Position = projectionMatrix * viewMatrix * position;\n"
                "}\n",

                SHADER_DEMO_HEADER
               #if JUCE_OPENGL_ES
                "varying highp float lightIntensity;\n"
               #else
                "varying float lightIntensity;\n"
               #endif
                "\n"
                "void main (void)\n"
                "{\n"
               #if JUCE_OPENGL_ES
                "   highp float l = lightIntensity * 0.25;\n"
                "   highp vec4 colour = vec4 (l, l, l, 1.0);\n"
               #else
                "   float l = lightIntensity * 0.25;\n"
                "   vec4 colour = vec4 (l, l, l, 1.0);\n"
               #endif
                "\n"
                "    gl_FragColor = colour;\n"
                "}\n"
            },

            {
                "Flattened",

                SHADER_DEMO_HEADER
                "attribute vec4 position;\n"
                "attribute vec4 normal;\n"
                "\n"
                "uniform mat4 projectionMatrix;\n"
                "uniform mat4 viewMatrix;\n"
                "uniform vec4 lightPosition;\n"
                "\n"
                "varying float lightIntensity;\n"
                "\n"
                "void main (void)\n"
                "{\n"
                "    vec4 light = viewMatrix * lightPosition;\n"
                "    lightIntensity = dot (light, normal);\n"
                "\n"
                "    vec4 v = vec4 (position);\n"
                "    v.z = v.z * 0.1;\n"
                "\n"
                "    gl_Position = projectionMatrix * viewMatrix * v;\n"
                "}\n",

                SHADER_DEMO_HEADER
               #if JUCE_OPENGL_ES
                "varying highp float lightIntensity;\n"
               #else
                "varying float lightIntensity;\n"
               #endif
                "\n"
                "void main (void)\n"
                "{\n"
               #if JUCE_OPENGL_ES
                "   highp float l = lightIntensity * 0.25;\n"
                "   highp vec4 colour = vec4 (l, l, l, 1.0);\n"
               #else
                "   float l = lightIntensity * 0.25;\n"
                "   vec4 colour = vec4 (l, l, l, 1.0);\n"
               #endif
                "\n"
                "    gl_FragColor = colour;\n"
                "}\n"
            },

            {
                "Toon Shader",

                SHADER_DEMO_HEADER
                "attribute vec4 position;\n"
                "attribute vec4 normal;\n"
                "\n"
                "uniform mat4 projectionMatrix;\n"
                "uniform mat4 viewMatrix;\n"
                "uniform vec4 lightPosition;\n"
                "\n"
                "varying float lightIntensity;\n"
                "\n"
                "void main (void)\n"
                "{\n"
                "    vec4 light = viewMatrix * lightPosition;\n"
                "    lightIntensity = dot (light, normal);\n"
                "\n"
                "    gl_Position = projectionMatrix * viewMatrix * position;\n"
                "}\n",

                SHADER_DEMO_HEADER
               #if JUCE_OPENGL_ES
                "varying highp float lightIntensity;\n"
               #else
                "varying float lightIntensity;\n"
               #endif
                "\n"
                "void main (void)\n"
                "{\n"
               #if JUCE_OPENGL_ES
                "    highp float intensity = lightIntensity * 0.5;\n"
                "    highp vec4 colour;\n"
               #else
                "    float intensity = lightIntensity * 0.5;\n"
                "    vec4 colour;\n"
               #endif
                "\n"
                "    if (intensity > 0.95)\n"
                "        colour = vec4 (1.0, 0.5, 0.5, 1.0);\n"
                "    else if (intensity > 0.5)\n"
                "        colour  = vec4 (0.6, 0.3, 0.3, 1.0);\n"
                "    else if (intensity > 0.25)\n"
                "        colour  = vec4 (0.4, 0.2, 0.2, 1.0);\n"
                "    else\n"
                "        colour  = vec4 (0.2, 0.1, 0.1, 1.0);\n"
                "\n"
                "    gl_FragColor = colour;\n"
                "}\n"
            }
        };

        return Array<ShaderPreset> (presets, numElementsInArray (presets));
    }

	static Leap::Controller& getController() 
    {
        static Leap::Controller s_controller;

        return  s_controller;
    }
};

// This static object will register this demo type in a global list of demos..
static JuceDemoType<OpenGLDemoClasses::OpenGLDemo> demo ("20 Graphics: OpenGL");
