#include <emscripten.h>
#include <SDL2/SDL.h>
#include <osgEarth/Registry>
#include <osgEarth/MapNode>
#include <osgEarthUtil/Sky>
#include <osgEarthUtil/EarthManipulator>
#include <pipeline/Predefines.h>
#include "osgearth_viewer.h"

USE_OSG_PLUGINS()
USE_VERSE_PLUGINS()
USE_OSGPLUGIN(earth)
USE_OSGPLUGIN(osgearth_xyz)
USE_OSGPLUGIN(osgearth_engine_mp)
USE_SERIALIZER_WRAPPER(DracoGeometry)
USE_GRAPICSWINDOW_IMPLEMENTATION(SDL)

osg::ref_ptr<Application> g_app = new Application;
void loop() { g_app->frame(); }

int main(int argc, char** argv)
{
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv);
    osgEarth::Registry::instance()->overrideTerrainEngineDriverName() = "mp";

    osg::ref_ptr<osg::Group> root = new osg::Group;
    g_app->scripter()->setRootNode(root.get());

    // Create the viewer
    osgViewer::Viewer* viewer = new osgViewer::Viewer;
    viewer->getCamera()->setLODScale(viewer->getCamera()->getLODScale() * 1.5f);
	viewer->getDatabasePager()->setIncrementalCompileOperation(new osgUtil::IncrementalCompileOperation());
	viewer->getDatabasePager()->setDoPreCompile(true);
	viewer->getDatabasePager()->setTargetMaximumNumberOfPageLOD(0);
	viewer->getDatabasePager()->setUnrefImageDataAfterApplyPolicy(true, true);
    viewer->addEventHandler(new osgViewer::StatsHandler);
	viewer->setCameraManipulator(new osgEarth::Util::EarthManipulator);
    viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer->setSceneData(root.get());

    // Load earth data
    std::stringstream ss;
    ss << "<map name=\"OpenStreetMap\" type=\"geocentric\" version=\"2\">\n"
       << "  <image name=\"osm_mapnik\" driver=\"xyz\">\n"
       << "    <url>http://[abc].tile.openstreetmap.org/{z}/{x}/{y}.png</url>\n"
       << "    <profile>spherical-mercator</profile><cache_policy usage=\"none\"/>\n"
       << "    <attribution>OpenStreetMap contributors</attribution>\n"
       << "  </image><options>\n"
       << "    <lighting>false</lighting>\n"
       << "    <terrain><min_tile_range_factor>8</min_tile_range_factor></terrain>\n"
       << "  </options></map>";
    osgDB::ReaderWriter* rw = osgDB::Registry::instance()->getReaderWriterForExtension("earth");
    osg::ref_ptr<osg::Node> earthRoot = rw ? rw->readNode(ss).getNode() : NULL;
    if (earthRoot.valid())
    {
        osg::ref_ptr<osgEarth::Util::MapNode> mapNode = osgEarth::Util::MapNode::findMapNode(earthRoot.get());
        if (!mapNode) { OSG_WARN << "Loaded scene graph does not contain a MapNode" << std::endl; }
        else root->addChild(mapNode.get());
    }

    // Create the graphics window
    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
    traits->x = 0; traits->y = 0; traits->width = 800; traits->height = 600;
    traits->alpha = 8; traits->depth = 24; traits->stencil = 8;
    traits->windowDecoration = true; traits->doubleBuffer = true;
    traits->readDISPLAY(); traits->setUndefinedScreenDetailsToDefaultScreen();
    traits->windowingSystemPreference = "SDL";

    osg::ref_ptr<osg::GraphicsContext> gc = osg::GraphicsContext::createGraphicsContext(traits.get());
    viewer->getCamera()->setGraphicsContext(gc.get());
    viewer->getCamera()->setViewport(0, 0, traits->width, traits->height);
    viewer->getCamera()->setDrawBuffer(GL_BACK);
    viewer->getCamera()->setReadBuffer(GL_BACK);

    // Start the main loop
    atexit(SDL_Quit);
    g_app->setViewer(viewer);
    emscripten_set_main_loop(loop, -1, 0);
    return 0;
}