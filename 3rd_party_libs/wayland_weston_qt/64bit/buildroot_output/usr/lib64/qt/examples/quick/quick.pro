TEMPLATE = subdirs
SUBDIRS =   quick-accessibility \
            animation \
            draganddrop \
            externaldraganddrop \
            canvas \
            imageelements \
            keyinteraction \
            localstorage \
            models \
            views \
            mousearea \
            positioners \
            righttoleft \
            scenegraph \
            shadereffects \
            text \
            textureprovider \
            threading \
            touchinteraction \
            tutorials \
            customitems \
            imageprovider \
            imageresponseprovider \
            window \
            particles \
            demos \
            rendercontrol

# Widget dependent examples
qtHaveModule(widgets) {
    SUBDIRS += embeddedinwidgets
    # OpenGL Support Required
    qtConfig(opengl(es1|es2)?) {
        qtHaveModule(quickwidgets): SUBDIRS += quickwidgets
    }
}

EXAMPLE_FILES = \
    shared
