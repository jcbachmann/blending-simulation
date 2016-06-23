import qbs

Application {
	files: [ "**.h", "**.cpp" ]

	Depends { name: "cpp" }

	cpp.cxxFlags: [ "-std=gnu++11", "-pthread" ].concat(
		qbs.buildVariant == "release" ? ["-O3"] : [])
	cpp.linkerFlags: [ "-std=gnu++11", "-pthread" ]

	consoleApplication: false

	Group {
		fileTagsFilter: "application"
		qbs.install: true
		qbs.installDir: "."
	}
}
