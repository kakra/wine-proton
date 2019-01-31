#!/usr/bin/env ruby

require 'yaml'

# Dependency walker to expand each patch series into a list of patch files
class Dependencies

  def self.expand_patch series
    series.split.each do |source_directory|
      files = File.join "patches", source_directory, "*.patch"
      puts "", "# expanded from #{files}"
      puts Dir.glob(files)
    end
  end

  def self.walk dependency_tree
    dependency_tree.each_pair do |series,wanted_by|
      expand_patch series
      walk wanted_by unless wanted_by.empty?
    end
  end
end

puts "#!/usr/bin/env git am"
patches = Dir.glob('docs/patches/*.yml').inject({}) do |all,series|
  all.merge! YAML.load_file(series)
end

Dependencies.walk patches
