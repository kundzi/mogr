//
//  MogrView.m
//  mogr_ios
//
//  Created by Dmitry Kunin on 23.12.13.
//  Copyright (c) 2013 mapswithme. All rights reserved.
//

#import "MogrView.h"
#import "renderer.h"

@implementation MogrView
{
  GlRenderer * m_renderer;
}

// You must implement this method
+ (Class)layerClass
{
  return [CAEAGLLayer class];
}


- (id)initWithFrame:(CGRect)frame
{
  NSLog(@"View is inited.");
  m_renderer = NULL;
  
  self = [super initWithFrame:frame];
  if (self)
  {
    // Setup Layer Properties
    CAEAGLLayer * eaglLayer = (CAEAGLLayer *)self.layer;
    
    eaglLayer.opaque = NO;
    
    // ColorFormat : RGB565
    // Backbuffer : YES, (to prevent from loosing content when mixing with ordinary layers).
    eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
                                    [NSNumber numberWithBool:NO],
                                    kEAGLDrawablePropertyRetainedBacking,
                                    kEAGLColorFormatRGB565,
                                    kEAGLDrawablePropertyColorFormat,
                                    nil];
    // Correct retina display support in opengl renderbuffer
    self.contentScaleFactor = [[UIScreen mainScreen] scale];
  }
  return self;
}

-(void)layoutSubviews
{
  if (m_renderer != NULL)
  {
    m_renderer->Stop();
    delete m_renderer;
  }
  
  m_renderer = new GlRenderer;
  m_renderer->Start((CAEAGLLayer *)self.layer);
}

@end
