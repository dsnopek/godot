/*************************************************************************/
/*  webxr.js                                                             */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2020 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2020 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

// Uses setTimeout() so that we can access library objects from Emscripten
// after they have been initialized.
setTimeout(function () {
    Module.Library_GL = GL || {};
    Module.Library_Browser = Browser || {};
    Module.Library_Browser_mainLoop = Module.Library_Browser.mainLoop || {};

    Module.webxr_texture_ids = [null, null];
    Module.webxr_textures = [null, null];

    Module.webxr_session = null;
    Module.webxr_space = null;
    Module.webxr_frame = null;
    Module.webxr_pose = null;

    // Monkey-patch the requestAnimationFrame() used by Emscripten for the main
    // loop, so that we can swap it out for XRSession.requestAnimationFrame()
    // when an XR session is started.
    Module.webxr_orig_requestAnimationFrame = Module.Library_Browser.requestAnimationFrame;
    Module.Library_Browser.requestAnimationFrame = function (callback) {
        if (Module.webxr_session && Module.webxr_space) {
            let onFrame = function (time, frame) {
                // @todo Do we actually need to do this?
                Module.webxr_session = frame.session;

                Module.webxr_frame = frame;
                Module.webxr_pose = frame.getViewerPose(Module.webxr_space);
                callback(time);
                Module.webxr_frame = null;
                Module.webxr_pose = null;
            };
            Module.webxr_session.requestAnimationFrame(onFrame);
        }
        else {
            Module.webxr_orig_requestAnimationFrame(callback);
        }
    };
}, 0);
