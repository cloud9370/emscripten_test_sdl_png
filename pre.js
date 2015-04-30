if(!Module) Module = {};
Module['preRun'] = [
	function (){
        console.log('call from Module.preRun');
		ENV.CHEWING_PATH = "/libchewing";
	}
];