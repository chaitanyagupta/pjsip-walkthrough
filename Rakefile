output = "~/Sites/pjsip-walkthrough.html"

task :default => [output]

file output => ["index.md"] do |t|
     sh "remark-generate-slides #{t.prerequisites[0]} #{t.name}"
end
