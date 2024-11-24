// Internal Includes ------------------------------
#include "GlobalInclude.cpp"

namespace Magnum {
    
    namespace Examples {
        using namespace Math::Literals;

        // Scene Objects -----------------------------------
        typedef SceneGraph::Object<SceneGraph::MatrixTransformation3D> Object3D;
        typedef SceneGraph::Scene<SceneGraph::MatrixTransformation3D> Scene3D;


        // Base Application --------------------------------
        class ImpressionEngine : public Platform::Application {
        public:
            explicit ImpressionEngine(const Arguments& arguments);

        private:
            void drawEvent() override;
            void viewportEvent(ViewportEvent& event) override;
            void pointerPressEvent(PointerEvent& event) override;
            void pointerReleaseEvent(PointerEvent& event) override;
            void pointerMoveEvent(PointerMoveEvent& event) override;
            void scrollEvent(ScrollEvent& event) override;

            Vector3 positionOnSphere(const Vector2& position) const;
            
            Shaders::PhongGL _coloredShader;
            Shaders::PhongGL _texturedShader{ Shaders::PhongGL::Configuration{}
                .setFlags(Shaders::PhongGL::Flag::DiffuseTexture| Shaders::PhongGL::Flag::NormalTexture) };
            Containers::Array<Containers::Optional<GL::Mesh>> _meshes;
            Containers::Array<Containers::Optional<GL::Texture2D>> _textures;

            Scene3D _scene;
            Object3D _manipulator, _cameraObject;
            SceneGraph::Camera3D* _camera;
            SceneGraph::DrawableGroup3D _drawables, _drawables1;
            Vector3 _previousPosition;
        };



        // Scene Objects -----------------------------------
        class ColoredDrawable : public SceneGraph::Drawable3D {
        public:
            explicit ColoredDrawable(Object3D& object, Shaders::PhongGL& shader, GL::Mesh& mesh, const Color4& color, SceneGraph::DrawableGroup3D& group) : SceneGraph::Drawable3D{ object, &group }, _shader(shader), _mesh(mesh), _color{ color } {}

        private:
            void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) override;

            Shaders::PhongGL& _shader;
            GL::Mesh& _mesh;
            Color4 _color;
        };

        class TexturedDrawable : public SceneGraph::Drawable3D {
        public:
            explicit TexturedDrawable(Object3D& object, Shaders::PhongGL& shader, GL::Mesh& mesh, GL::Texture2D& texture, SceneGraph::DrawableGroup3D& group) : SceneGraph::Drawable3D{ object, &group }, _shader(shader), _mesh(mesh), _texture(texture) {}

        private:
            void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) override;

            Shaders::PhongGL& _shader;
            GL::Mesh& _mesh;
            GL::Texture2D& _texture;
        };

        // Implementation Functions --------------------------
        ImpressionEngine::ImpressionEngine(const Arguments& arguments) :
            Platform::Application{ arguments, Configuration{}
                .setTitle("Magnum Viewer Example")
                .setWindowFlags(Configuration::WindowFlag::Resizable) }
        {

            //FIXME: figure out how commandlines work

            Utility::Arguments args;
            args.addArgument("file").setHelp("file", "file to load")
                .addOption("importer", "AnySceneImporter")
                .setHelp("importer", "importer plugin to use")
                .addSkippedPrefix("magnum", "engine-specific options")
                .setGlobalHelp("Displays a 3D scene file provided on command line.");
            args.parse(arguments.argc, arguments.argv);


            _cameraObject
                .setParent(&_scene)
                .translate(Vector3::zAxis(5.0f));
            (*(_camera = new SceneGraph::Camera3D{ _cameraObject }))
                .setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
                .setProjectionMatrix(
                    Matrix4::perspectiveProjection(35.0_degf, 1.0f, 0.01f, 1000.0f)
                )
                .setViewport(GL::defaultFramebuffer.viewport().size());

            _manipulator.setParent(&_scene);

            /* Setup renderer and shader defaults */
            GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
            GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
            _coloredShader
                .setAmbientColor(0x111111_rgbf)
                .setSpecularColor(0xffffff_rgbf)
                .setShininess(80.0f);
            _texturedShader
                .setAmbientColor(0x111111_rgbf)
                .setSpecularColor(0x111111_rgbf)
                .setShininess(80.0f);

            /* Load a scene importer plugin */
            PluginManager::Manager<Trade::AbstractImporter> manager;
            Containers::Pointer<Trade::AbstractImporter> importer =
                manager.loadAndInstantiate("AnySceneImporter");


            if (!importer->openFile(args.value("file")))
                // FIXME: This is a hard code. Swap this out for your local machine directory. 
                //if (!importer->openFile("C:/Users/violi/Downloads/magnum-bootstrap-base/magnum-bootstrap-base/assets/scene.glb"))
                std::exit(1);
            if (!importer)
                std::exit(1);

            /* Load all textures. Textures that fail to load will be NullOpt. */
            _textures = Containers::Array<Containers::Optional<GL::Texture2D>>{
                importer->textureCount() };
            for (UnsignedInt i = 0; i != importer->textureCount(); ++i) {
                Containers::Optional<Trade::TextureData> textureData =
                    importer->texture(i);
                if (!textureData || textureData->type() != Trade::TextureType::Texture2D) {
                    Warning{} << "Cannot load texture" << i
                        << importer->textureName(i);
                    continue;
                }

                Containers::Optional<Trade::ImageData2D> imageData =
                    importer->image2D(textureData->image());
                /*if(!imageData || !imageData->isCompressed()) {
                    Warning{} << "Cannot load image" << textureData->image()
                        << importer->image2DName(textureData->image());
                    continue;
                }*/

                (*(_textures[i] = GL::Texture2D{}))
                    .setMagnificationFilter(textureData->magnificationFilter())
                    .setMinificationFilter(textureData->minificationFilter(),
                        textureData->mipmapFilter())
                    .setWrapping(textureData->wrapping().xy())
                    .setStorage(Math::log2(imageData->size().max()) + 1,
                        GL::textureFormat(imageData->format()), imageData->size())
                    .setSubImage(0, {}, *imageData)
                    .generateMipmap();
            }

            /* Load all materials. Materials that fail to load will be NullOpt. Only a
               temporary array as the material attributes will be stored directly in
               drawables later. */
            Containers::Array<Containers::Optional<Trade::PhongMaterialData>> materials{
                importer->materialCount() };
            for (UnsignedInt i = 0; i != importer->materialCount(); ++i) {
                Containers::Optional<Trade::MaterialData> materialData;
                if (!(materialData = importer->material(i))) {
                    Warning{} << "Cannot load material" << i
                        << importer->materialName(i);
                    continue;
                }

                materials[i] = std::move(*materialData).as<Trade::PhongMaterialData>();
            }

            /* Load all meshes. Meshes that fail to load will be NullOpt. Generate
               normals if not present. */
            _meshes = Containers::Array<Containers::Optional<GL::Mesh>>{
                importer->meshCount() * 2};
            for (UnsignedInt i = 0; i != importer->meshCount(); ++i) {
                Containers::Optional<Trade::MeshData> meshData;
                if (!(meshData = importer->mesh(i))) {
                    Warning{} << "Cannot load mesh" << i << importer->meshName(i);
                    continue;
                }

                MeshTools::CompileFlags flags;
                if (!meshData->hasAttribute(Trade::MeshAttribute::Normal))
                   flags |= MeshTools::CompileFlag::GenerateFlatNormals;
                _meshes[i] = MeshTools::compile(*meshData, flags);

                // Duplicate mesh and add it to the list
                 // Invert normals for the duplicate mesh
                //if (meshData->hasAttribute(Trade::MeshAttribute::Normal)) {
                //    // Access the normal data
                //    Containers::StridedArrayView1D<Vector3> normals =
                //        meshData->mutableAttribute<Vector3>(Trade::MeshAttribute::Normal);

                //     //Invert each normal
                //    for (Vector3& normal : normals) {
                //        normal = -normal;  // Invert the normal direction
                //    }

                //}

                _meshes[i + importer->meshCount()] = MeshTools::compile(*meshData, flags);
            }

            /* The format has no scene support, display just the first loaded mesh with
               a default material (if it's there) and be done with it. */
            if (importer->defaultScene() == -1) {
                if (!_meshes.isEmpty() && _meshes[0])
                    new ColoredDrawable{ _manipulator, _coloredShader, *_meshes[0],
                        0xffffff_rgbf, _drawables };
                return;
            }

            /* Load the scene */
            Containers::Optional<Trade::SceneData> scene;
            if (!(scene = importer->scene(importer->defaultScene())) ||
                !scene->is3D() ||
                !scene->hasField(Trade::SceneField::Parent) ||
                !scene->hasField(Trade::SceneField::Mesh))
            {
                Fatal{} << "Cannot load scene" << importer->defaultScene()
                    << importer->sceneName(importer->defaultScene());
            }

            /* Allocate objects that are part of the hierarchy */
            Containers::Array<Object3D*> objects{ std::size_t(scene->mappingBound()) };
            Containers::Array<Containers::Pair<UnsignedInt, Int>> parents = scene->parentsAsArray();

            for (const Containers::Pair<UnsignedInt, Int>& parent : parents)
                objects[parent.first()] = new Object3D{};

            for (const Containers::Pair<UnsignedInt, Int>& parent : parents)
                objects[parent.first()]->setParent(parent.second() == -1 ?
                    &_manipulator : objects[parent.second()]);

            // Adjust transformations
            for (const Containers::Pair<UnsignedInt, Matrix4>& transformation : scene->transformations3DAsArray())
                if (Object3D* object = objects[transformation.first()])
                    object->setTransformation(transformation.second());

            // Add drawables and position them explicitly
            
           
            for (const Containers::Pair<UnsignedInt, Containers::Pair<UnsignedInt, Int>>& meshMaterial : scene->meshesMaterialsAsArray()) {
              
                Object3D* object = objects[meshMaterial.first()];
                Containers::Optional<GL::Mesh>& mesh = _meshes[meshMaterial.second().first()];
                if (!object || !mesh) continue;

                Int materialId = meshMaterial.second().second();

                

                // Create a duplicate object and translate it
                Object3D* duplicateObject = new Object3D{ &_manipulator };
                //duplicateObject->translate(Vector3::xAxis(1.5f)); // Move the duplicate mesh to the right
                // Compute the world transformation of the original object
                Matrix4 worldTransformation = object->transformationMatrix();
                Object3D* parent = object->parent();
                while (parent) {
                    worldTransformation = parent->transformationMatrix() * worldTransformation;
                    parent = parent->parent();
                }

                // Apply the world transformation to the duplicate object
                duplicateObject->setTransformation(worldTransformation);
                duplicateObject->scale(Vector3(1.05f, 1.05f, 1.05f));
                // Create drawables for both original and duplicate meshes
                if (materialId == -1 || !materials[materialId]) {
                    new ColoredDrawable{ *object, _coloredShader, *mesh, 0xffffff_rgbf, _drawables };
                    new ColoredDrawable{ *duplicateObject, _coloredShader, *_meshes[meshMaterial.second().first() + importer->meshCount()], 0x000000_rgbf, _drawables1 };
                }
               else if (materials[materialId]->hasAttribute(Trade::MaterialAttribute::DiffuseTexture)
                    && _textures[materials[materialId]->diffuseTexture()]) {
                    new TexturedDrawable{ *object, _texturedShader, *mesh,
                        *_textures[materials[materialId]->diffuseTexture()],
                        _drawables };
                    new ColoredDrawable{ *duplicateObject, _coloredShader, *_meshes[meshMaterial.second().first() + importer->meshCount()],
                        0x000000_rgbf, _drawables1 };
                   
                }
                else if (materials[materialId]->hasAttribute(Trade::MaterialAttribute::NormalTexture)
                    && _textures[materials[materialId]->normalTexture()]) {
                    new TexturedDrawable{ *object, _texturedShader, *mesh,
                        *_textures[materials[materialId]->normalTexture()],
                        _drawables };
                    new ColoredDrawable{ *duplicateObject, _coloredShader, *_meshes[meshMaterial.second().first() + importer->meshCount()],
                        0x000000_rgbf, _drawables1 };
                    
                }
                else {        
                    new ColoredDrawable{ *object, _coloredShader, *mesh,
                        materials[materialId]->diffuseColor(), _drawables };
                    new ColoredDrawable{ *duplicateObject, _coloredShader, *_meshes[meshMaterial.second().first() + importer->meshCount()],
                        0x000000_rgbf, _drawables1 };
                }
            }
        }

        void ColoredDrawable::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {
           
            _shader
                .setDiffuseColor(_color)
                .setLightPositions({
                    {camera.cameraMatrix().transformPoint({3.0f, 2000.0f, 2000.0f}), 0.0f}
                    })
                .setTransformationMatrix(transformationMatrix)
                .setNormalMatrix(transformationMatrix.normalMatrix())
                .setProjectionMatrix(camera.projectionMatrix())
                .draw(_mesh);



        }

        void TexturedDrawable::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {
            _shader
                .setLightPositions({
                    {camera.cameraMatrix().transformPoint({3.0f, 2000.0f, 2000.0f}), 0.0f}
                    })
                .setTransformationMatrix(transformationMatrix)
                .setNormalMatrix(transformationMatrix.normalMatrix())
                .setProjectionMatrix(camera.projectionMatrix())
                .bindDiffuseTexture(_texture)
                .bindNormalTexture(_texture)
                .draw(_mesh);
        }

        // Event Definitions -------------------------------
        void ImpressionEngine::drawEvent() {
            GL::defaultFramebuffer.clear(GL::FramebufferClear::Color |
                GL::FramebufferClear::Depth);

            GL::Renderer::setFaceCullingMode(GL::Renderer::PolygonFacing::Front);
            _camera->draw(_drawables1);
            GL::Renderer::setFaceCullingMode(GL::Renderer::PolygonFacing::Back);
            _camera->draw(_drawables);

            swapBuffers();
        }

        void ImpressionEngine::viewportEvent(ViewportEvent& event) {
            GL::defaultFramebuffer.setViewport({ {}, event.framebufferSize() });
            _camera->setViewport(event.windowSize());
        }

        void ImpressionEngine::pointerPressEvent(PointerEvent& event) {
            if (!event.isPrimary() ||
                !(event.pointer() & (Pointer::MouseLeft | Pointer::Finger)))
                return;

            _previousPosition = positionOnSphere(event.position());
        }

        void ImpressionEngine::pointerReleaseEvent(PointerEvent& event) {
            if (!event.isPrimary() ||
                !(event.pointer() & (Pointer::MouseLeft | Pointer::Finger)))
                return;

            _previousPosition = {};
        }

        void ImpressionEngine::scrollEvent(ScrollEvent& event) {
            if (!event.offset().y()) return;

            /* Distance to origin */
            const Float distance = _cameraObject.transformation().translation().z();

            /* Move 15% of the distance back or forward */
            _cameraObject.translate(Vector3::zAxis(
                distance * (1.0f - (event.offset().y() > 0 ? 1 / 0.85f : 0.85f))));

            redraw();
        }

        Vector3 ImpressionEngine::positionOnSphere(const Vector2& position) const {
            const Vector2 positionNormalized =
                position / Vector2{ _camera->viewport() } - Vector2{ 0.5f };
            const Float length = positionNormalized.length();
            const Vector3 result = length > 1.0f ?
                Vector3{ positionNormalized, 0.0f } :
                Vector3{ positionNormalized, 1.0f - length };
            return (result * Vector3::yScale(-1.0f)).normalized();
        }

        void ImpressionEngine::pointerMoveEvent(PointerMoveEvent& event) {
            if (!event.isPrimary() ||
                !(event.pointers() & (Pointer::MouseLeft | Pointer::Finger)))
                return;

            const Vector3 currentPosition = positionOnSphere(event.position());
            const Vector3 axis = Math::cross(_previousPosition, currentPosition);

            if (_previousPosition.isZero() || axis.isZero())
                return;

            _manipulator.rotate(Math::angle(_previousPosition, currentPosition), axis.normalized());
            _previousPosition = currentPosition;

            redraw();
        }

    }
}

// Start Caller ------------------------------------
MAGNUM_APPLICATION_MAIN(Magnum::Examples::ImpressionEngine)
