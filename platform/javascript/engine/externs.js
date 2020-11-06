var Godot;
var WebAssembly = {};
WebAssembly.instantiate = function(buffer, imports) {};
// This doesn't actually work.
class XRWebGLLayer {
    constructor(session, ctx) {
        this.framebuffer;
    }
    getViewport(view) {}
};
// This doesn't work either.
/*
var XRWebGLLayer = function(session, ctx) {
    this.framebuffer;
};
XRWebGLLayer.prototype.getViewport = function(view) {};
*/
