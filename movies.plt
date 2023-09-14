#!/usr/bin/plutonium
import nuke
import json

var movies = {
  "names": [
    "The Cop, the gangster, the devil",
    "Behind enemy lines",
    "Top Gun",
    "The Divine Move",
    "Pawn Sacrifice",
    "The lord of war"
  ]
}
function listMovies(var req)
{
  println(req.getenv("CONTENT_TYPE"))
  println(req.getenv("QUERY_STRING"))
  return nuke.response("application/json",json.dumps(movies))
}

function deleteMovie(var req,var name)
{
  var idx = movies["names"].find(name)
  if(idx == nil)
    return nuke.response("application/json","{'msg': 'Unknown movie'}")
  
  movies["names"].erase(idx)
  return nuke.response("application/json","{'msg': 'Success'}")
}

function addMovie(var req,var name)
{
  println(req.getenv("CONTENT_TYPE"))
  println(req.getenv("CONTENT_LENGTH"))
  var idx = movies["names"].find(name)
  if(idx != nil)
    return nuke.response("application/json","{'msg': 'Movie already exists!'}")
  movies["names"].push(name)
  return nuke.response("application/json","{'msg' : 'Success'}")
}
function test_qstr(var req)
{
  println(req.args)
  return nuke.response("text/html","All ok!")
}
var app = nuke.app()
app.route("GET","/movies",listMovies)
app.route("POST","/movies/<name>",addMovie)
app.route("DELETE","/movies/<name>",deleteMovie)
app.route("GET","/testqstr",test_qstr)
app.run("127.0.0.1:2016",10)
