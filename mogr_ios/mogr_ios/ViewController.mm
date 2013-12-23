//
//  ViewController.m
//  mogr_ios
//
//  Created by Dmitry Kunin on 23.12.13.
//  Copyright (c) 2013 mapswithme. All rights reserved.
//

#import "ViewController.h"

#import "MogrView.h"

@interface ViewController ()

@end

@implementation ViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
  
  MogrView * view = [[MogrView alloc] initWithFrame:self.view.bounds];
  self.view.backgroundColor = [UIColor blueColor];
  [self.view addSubview:view];
  view.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

@end
