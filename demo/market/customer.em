system.import("std/library.em");
/* Make Customer send out a touch event to the Book entity */

/* question is how do you know the uuid of the Book entity */
/* In a real world you just know that the book is in your front, so just touching it generates an event with appr. params */

/* So, in our simulations, we will just register with the market saying anything that match the regexp *Book*, please give the id to me */

/* We will fill this in sometime */
var MARKET_CHANNEL = undefined;

/* Our pattern of interest is Books */
//var patternOfInterest = new system.Pattern("pattern", "/*Books*/");
var patternOfInterest = "Books";

/* create a subscription message */
var subsObj = {"name":"subscribe", "pattern":patternOfInterest};

var simulator = undefined;

function handleMarket(msg, sender)
{
  if(!MARKET_CHANNEL)
  {
    MARKET_CHANNEL = sender;    
    print("\n\nGot a new market\n\n");

    handleMarketReply <- [new util.Pattern("vendor"), new util.Pattern("banner"), new util.Pattern("init_proto")] <- MARKET_CHANNEL;
    //send the subscription

    subsObj -> MARKET_CHANNEL;
    print("\n\n Subscription for books sent to market \n\n");
  }
}

// argument here is a new visible object
function proxAddedCallback(new_addr_obj)
{
  
  var test_msg = new Object();
  test_msg.name = "get_protocol";
  
  //also register a callback for market. the msg should have a "protocol" field with value "Market"
  var p = new util.Pattern("protocol", "Market");
  
  handleMarket <- p <- new_addr_obj;
  print("\n\nRegistered a pattern\n\n");
  test_msg -> new_addr_obj;
}

// This is the callback for the reply from the market
function handleMarketReply(msg, sender)
{
  print("\n\n Got a reply from the market . The vendor is " + msg.vendor + "\n\n");
  //Do this inside a new context
  var newContext = system.create_context(system.presences[0], msg.vendor, true, true, true);
  
  msg.init_proto(msg.init_arg_1, msg.vendor, system.presences[0]);
  newContext.execute(msg.init_proto, msg.init_arg_1, msg.init_arg_2); 
}


// Set up the camera
system.onPresenceConnected( function(pres) {
                                
    system.print("startupCamera connected " + pres);
    system.print(system.presences.length);
    if (system.presences.length == 1)
    {
      //simulator = pres.runSimulation("ogregraphics");
      system.presences[0].onProxAdded(proxAddedCallback);
    }
});

system.onPresenceDisconnected( function() {
    system.print("startupCamera disconnected");
});




