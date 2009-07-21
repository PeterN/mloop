#! /usr/bin/env python
# encoding: utf-8

VERSION='0.0.1'
APPNAME='mloop'

srcdir = '.'
blddir = 'build'

def init():
	pass

def set_options(opt):
	opt.tool_options('compiler_cxx')

def configure(conf):
	conf.check_tool('compiler_cxx')
	conf.sub_config('src')

	env = conf.env.copy()
	env.set_variant('debug')
	conf.set_env_name('debug', env)
	conf.setenv('debug')
	conf.env.CXXFLAGS = ['-Wall', '-g']

def build(bld):
	bld.add_subdirs('src')

	for obj in bld.all_task_gen[:]:
		new_obj = obj.clone('debug')
