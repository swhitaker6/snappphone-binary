/*
 * Lets you host .wasm file on a different location than the directory of the 
 * .js file (which is the default expectation), for example if you want
 * to host it on a CDN.
 * 
 * The function receives the relative path to the .wasm file as configured by
 * the build process and a 'prefix' (the path to the .js file's directory)
 * and should return the actual URL.
 */
//Module['locateFile'] = function(path, prefix) {

//	if(path.endswith('.wasm')) return "https://dev.sharedocnow.com/storage/docs/"+path;

//	return prefix + path;

//}
//
//
/*
 * Called when abnormal program termination occurs. That can happen due
 * to the C method abort() being called, or due to to a fatal problem such
 * as being unable to fetch the wasm binary).
 *
 */
//Module['onAbort'] = function() {}

/*
 *Called when the runtime is fully initialized.
 * 
 */
//Module['onRuntimeInitialized'] = function() {}

/*
 * If set to true, the runtime is not shutdown after run() completes so you can 
 * continue to make function call..
 *
 */
//Module['noExitRuntime'] = true;

/*
 * If set to true, main() will not automatically be called. 
 *
 */
//Module['noInitialRun'] = true;


var pBuf;

var startGenTexture = function () {

    var env = false; // is webGL2.0
    var page = 1;
    
    var texture0;
    var texture1;
    var uniforms;
    var loader = new THREE.ImageBitmapLoader();
    
    loader.load('http://sharedocnow.com/images/eagle.png', function(imageBitmap){

        console.log('inside loader.load() callback function');

        var w = imageBitmap.width;
        var h = imageBitmap.height;
        var canvas = document.getElementById("canvas");

        canvas.width = w;
        canvas.height = h;

        var canvasCtx = canvas.GetContext("2d");
        canvasCtx.drawImage(imageBitmap,0,0);

        var imageDataObject = canvasCtx.getImageData(0,0,w,h);

        // var img = [ 
        //     0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
        //     0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a,
        //     0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c,
        //     0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51,
        //     0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11,
        //     0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef,
        //     0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17,
        //     0xad, 0x2b, 0x41, 0x7b, 0xe6, 0x6c, 0x37, 0x10
        // ];

        var buffer = new ArrayBuffer(64);
        var textureArray = new Uint8Array(buffer);

        var imgTypedArray = new Uint8Array(imageDataObject.data);
        // var imgTypedArray = new Uint8Array(img);

        console.log("Image Typed Array LENGTH:");
        console.log(imgTypedArray.length);

        console.log("Image Typed Array BYTES_PER_ELEMENT:");
        console.log(imgTypedArray.BYTES_PER_ELEMENT);

        // console.log("Image Typed Array DATA:");
        // console.log(imgTypedArray);
        
        var sz = imgTypedArray.length*imgTypedArray.BYTES_PER_ELEMENT;
        pBuf = Module._malloc(sz);
        Module.HEAP8.set(imgTypedArray, pBuf);
        Module.ccall('genTexture', 'number', ['number', 'number', 'number', 'boolean' ], [page, pBuf, sz, env]);
        for(i=0; i < sz; i++) {

            textureArray[i] = Module.getValue(pBuf+i, 'i8'); 

        }
    
        // console.log(textureArray);
    //  Module._free(pBuf);

    });





}



var doGenTexture = function () {

    var env = false; // is webGL2.0
    var page = 1; 

    var buffer = new ArrayBuffer(64);
    var textureArray = new Uint8Array(buffer);

   
    var sz = textureArray.length*textureArray.BYTES_PER_ELEMENT;
    // var pBuf = Module._malloc(sz);
    // Module.HEAP8.set(imgTypedArray, pBuf);
    // Module.HEAP8.writeArrayToMemory(imgTypedArray, buf);
    Module.ccall('genTexture', 'number', ['number', 'number', 'number', 'boolean' ], [page, pBuf, sz, env]);
    for(i=0; i < sz; i++) {

        textureArray[i] = Module.getValue(pBuf+i, 'i8');

    }
 
    console.log(textureArray);
    //  Module._free(pBuf);

}



var freeGenTexture = function () {

    var page = 1; 

     Module._free(pBuf);

}




// var genTexture = Module.cwrap('genTexture', 'number', [ 'nunber', 'number' ], [ 'page', 'buf' ]);
    


/*
 * Called before global initializers run.
 *
 */
//Module['preInit'] = function() {}

/*
 * Called right before calling run(), but after global initializers.
 *
 */
//Module['preRun'] = function() {}

/*
 * Called when something is printed to C stdout.
 *
 */
//Module['print'] = function(text) {

//	console.log(text);

//}
//
/*
 * Called when something is printed to C stderr.
 *
 */
//Module['printErr'] = function(err) {

//	console.log(err);

//}	

/*
 * An optional callback that the Emscripten runtime calls to perform the 
 * WebAssembly instantiation action.
 *
 * This callback should call 'successCallback' upon successfull completion.
 * Usefull when you have other custom asynchronous startup actions or downloads
 * that can be performed in parallel with the WebAssembly instantiation action.
 * (for an example see the file tests/manual_wasm_instantiate.html)
 */
//Module['instantiateWasm'] = function(imports, successCallback) {}



