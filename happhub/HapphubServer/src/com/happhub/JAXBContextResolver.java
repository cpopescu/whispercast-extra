package com.happhub;

import org.eclipse.persistence.jaxb.JAXBContextFactory;
import org.eclipse.persistence.jaxb.JAXBContextProperties;

import javax.ws.rs.ext.ContextResolver;
import javax.ws.rs.ext.Provider;
import javax.xml.bind.JAXBContext;
import java.util.*;
 
@Provider
public final class JAXBContextResolver implements ContextResolver<JAXBContext> {
    private JAXBContext context;
 
    public JAXBContextResolver() throws Exception {
      List<Object> bindings = new ArrayList<Object>();
      bindings.add(this.getClass().getResourceAsStream("/META-INF/eclipselink-oxm-api-accounts.xml"));
      bindings.add(this.getClass().getResourceAsStream("/META-INF/eclipselink-oxm-api-cameras.xml"));
      bindings.add(this.getClass().getResourceAsStream("/META-INF/eclipselink-oxm-api-login.xml"));
      bindings.add(this.getClass().getResourceAsStream("/META-INF/eclipselink-oxm-api-sessions.xml"));
      bindings.add(this.getClass().getResourceAsStream("/META-INF/eclipselink-oxm-api-signup.xml"));
      bindings.add(this.getClass().getResourceAsStream("/META-INF/eclipselink-oxm-model.xml"));

      Map<String, Object> properties = new HashMap<String, Object>();
      properties.put(JAXBContextProperties.OXM_METADATA_SOURCE, bindings);

      this.context = JAXBContextFactory.createContext(new Class[]{}, properties);
    }

    @Override
    public JAXBContext getContext(Class<?> objectType) {
      return this.context;
    }
}