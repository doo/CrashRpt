main();

function main()
{
	var fileName = "crashrpt\\src\\CrashRpt.vcproj";
	WScript.Interactive = true;

	// Instantiate a DOM object at run time.
	var dom = new ActiveXObject("msxml2.DOMDocument.6.0");
	dom.async = false;
	dom.resolveExternals = false;
	dom.load(fileName);
	
	var PlatformsNode = dom.selectSingleNode("//VisualStudioProject/Platforms");
	
	var i;
	for(i=0; i<PlatformsNode.childNodes.length; i++)
	{
		var child = PlatformsNode.childNodes(i);
		if(child.getAttribute("Name")=="x64")
		{	
			alert("found");
			PlatformsNode.removeChild(child);
			break;
		}
    }

	var result = dom.save(fileName);
	alert("result");
}

// Helper function
function alert(str)
{
   WScript.Echo(str);
}


