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
function listMovies()
{
  return nuke.response("application/json",json.dumps(movies))
}

function deleteMovie(var name)
{
  var idx = movies["names"].find(name)
  if(idx == nil)
    return nuke.response("application/json","{'msg': 'Unknown movie'}")
  
  movies["names"].erase(idx)
  return nuke.response("application/json","{'msg': 'Success'}")
}

function addMovie(var name)
{
  var idx = movies["names"].find(name)
  if(idx != nil)
    return nuke.response("application/json","{'msg': 'Movie already exists!'}")
  movies["names"].push(name)
  return nuke.response("application/json","{'msg' : 'Success'}")
}

var app = nuke.app()
app.route("GET","/movies",listMovies)
app.route("POST","/movies/<name>",addMovie)
app.route("DELETE","/movies/<name>",deleteMovie)

app.run("127.0.0.1:2016",10)
