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
Module['noExitRuntime'] = true;

/*
 * If set to true, main() will not automatically be called. 
 *
 */
Module['noInitialRun'] = true;

//var genTexture = Module.cwrap('_genTexture', 'number', [ 'nunber', 'number' ], [ 'page', 'buf' ]);


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



