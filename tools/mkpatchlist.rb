#!/usr/bin/env ruby

require 'yaml'

# Dependency walker to expand each patch series into a list of patch files
class Dependencies

  def self.expand_patch series
    series.split.each do |source_directory|
      dir = File.join "patches", source_directory
      files = File.join dir, "*.patch"
      puts "# expanded from #{files}"
      begin
        definition = File.join dir, "definition"
        File.read(definition).each_line do |line|
          next if line =~ /^(#|Depends:)/
          puts "# #{line}"
        end
      rescue Errno::ENOENT
      end
      puts Dir.glob(files).sort
      puts ""
    end
  end

  def self.walk dependency_tree
    dependency_tree.each_pair do |series,wanted_by|
      expand_patch series
      walk wanted_by unless wanted_by.empty?
    end
  end
end

patches = Dir.glob('docs/patches/*.yml').inject({}) do |all,series|
  all.merge! YAML.load_file(series)
end

Dependencies.walk patches
