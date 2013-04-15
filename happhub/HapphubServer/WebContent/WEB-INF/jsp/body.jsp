<%@ taglib uri="http://java.sun.com/jsp/jstl/core" prefix="c" %>
<style>
  #carousel {
    margin: 0;
  }
  #carousel .item {
    background: #c0c0c0;
    background: rgba(0, 0, 0, 0.5);
  }
  #carousel .item.header1,
  #carousel .item.header2,
  #carousel .item.header3 {
    background-image: url('/img/carousel-1.png');
  }
  
  #carousel .carousel-caption {
    position: static;
    background: transparent;
    margin: 75px 50px 0 50px;
    padding-bottom: 30px;
  }
  #carousel .carousel-caption h3,
  #carousel .carousel-caption .lead {
    line-height: 1.25;
    color: #fff;
    text-shadow: 0 1px 1px rgba(0, 0, 0, 0.5);
  }
  #carousel .carousel-caption h3 {
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
  }
  #carousel .carousel-caption .lead {
    max-width: 600px;
    margin-bottom: 1em;
  }
  #carousel .carousel-control {
    background: transparent;
    border: none;
    font-size: 120px;
    top: 50%;
    width: 35px;
    height: 70px;
    line-height: 50px;
  }
</style> 
<div class="img-polaroid span10 offset1">
<div id="carousel" class="carousel slide">
  <div class="carousel-inner">
    <div class="item header1 active">
      <div class="container">
        <div class="carousel-caption">
          <h3>Live streaming, everywhere.</h3>
          <p class="lead">Use multiple local or remote cameras or mobile devices to cover each and every angle of your live event.</p>
          <a class="btn btn-success btn-large" href="/signup/">Sign Up</a>
        </div>
      </div>
    </div>
    <div class="item header2">
      <div class="container">
        <div class="carousel-caption">
          <h3>Flexible on-the-run editing.</h3>
          <p class="lead">Edit your output video stream by switching between your local cameras, remote cameras or recorded video.</p>
          <a class="btn btn-success btn-large" href="/signup/">Sign up</a>
        </div>
      </div>
    </div>
    <div class="item header3">
      <div class="container">
        <div class="carousel-caption">
          <h3>Share your live experience.</h3>
          <p class="lead">Share the live video stream with your community and let them interact and be part of it.</p>
          <a class="btn btn-success btn-large" href="signup/">Sign up</a>
        </div>
      </div>
    </div>
  </div>
  <a class="left carousel-control" href="#carousel" data-slide="prev">&lsaquo;</a>
  <a class="right carousel-control" href="#carousel" data-slide="next">&rsaquo;</a>
</div>
</div>