package com.happhub;

import java.util.HashSet;
import java.util.Set;

import org.eclipse.persistence.jaxb.rs.MOXyJsonProvider;

public class Application extends javax.ws.rs.core.Application {
  @Override
  public Set<Object> getSingletons() {
      MOXyJsonProvider moxyJsonProvider = new MOXyJsonProvider();

      moxyJsonProvider.setAttributePrefix(null);
      moxyJsonProvider.setFormattedOutput(true);
      moxyJsonProvider.setIncludeRoot(false);
      moxyJsonProvider.setMarshalEmptyCollections(true);
      moxyJsonProvider.setValueWrapper(null);

      HashSet<Object> set = new HashSet<Object>(1);
      set.add(moxyJsonProvider);
      return set;
  }
}
